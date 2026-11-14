#include "dataset.h"
#include "hull_app.h"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <random>
#include <sstream>

namespace {

constexpr unsigned WINDOW_W = 1200;
constexpr unsigned WINDOW_H = 760;
constexpr float MARGIN = 20.f;
constexpr float PLOT = 720.f;  // the world square [0, WORLD_SIZE]^2 is drawn as PLOT x PLOT pixels
constexpr float PANEL_X = 2 * MARGIN + PLOT;
constexpr std::size_t RANDOM_POINTS = 30;
const char* const FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
const int SPEEDS[] = {1, 2, 5, 10, 20, 50, 100, 200, 500, 1000};  // steps per second
constexpr int SPEED_COUNT = sizeof(SPEEDS) / sizeof(SPEEDS[0]);

namespace color {
const sf::Color background(24, 26, 33);
const sf::Color plot(32, 35, 44);
const sf::Color border(70, 75, 90);
const sf::Color point(150, 160, 180);
const sf::Color hull(80, 200, 140);
const sf::Color hullFill(80, 200, 140, 40);
const sf::Color tested(255, 170, 60);
const sf::Color current(255, 230, 80);
const sf::Color rejected(240, 80, 80);
const sf::Color text(220, 224, 232);
const sf::Color dim(140, 146, 160);
}  // namespace color

sf::Vector2f toScreen(const Point& p) {
    const float s = PLOT / static_cast<float>(WORLD_SIZE);
    return {MARGIN + static_cast<float>(p.x) * s, MARGIN + static_cast<float>(WORLD_SIZE - p.y) * s};
}

Point toWorld(sf::Vector2f v) {
    const double s = WORLD_SIZE / PLOT;
    return {(v.x - MARGIN) * s, WORLD_SIZE - (v.y - MARGIN) * s};
}

bool insidePlot(sf::Vector2f v) {
    return v.x >= MARGIN && v.x <= MARGIN + PLOT && v.y >= MARGIN && v.y <= MARGIN + PLOT;
}

// Thick line as a quad (sf::Lines is always 1px wide).
void addLine(sf::VertexArray& quads, sf::Vector2f a, sf::Vector2f b, float width, sf::Color c) {
    const sf::Vector2f d = b - a;
    const float len = std::sqrt(d.x * d.x + d.y * d.y);
    if (len < 1e-4f) return;
    const sf::Vector2f n(-d.y / len * width / 2, d.x / len * width / 2);
    quads.append({a + n, c});
    quads.append({b + n, c});
    quads.append({b - n, c});
    quads.append({a - n, c});
}

void addDot(sf::VertexArray& quads, sf::Vector2f p, float r, sf::Color c) {
    quads.append({{p.x - r, p.y - r}, c});
    quads.append({{p.x + r, p.y - r}, c});
    quads.append({{p.x + r, p.y + r}, c});
    quads.append({{p.x - r, p.y + r}, c});
}

std::string wrap(const std::string& text, std::size_t width) {
    std::istringstream words(text);
    std::string word, line, result;
    while (words >> word) {
        if (!line.empty() && line.size() + 1 + word.size() > width) {
            result += line + '\n';
            line.clear();
        }
        line += (line.empty() ? "" : " ") + word;
    }
    return result + line;
}

class Visualizer {
public:
    explicit Visualizer(const HullAlgorithm& algo)
        : algo_(algo), window_(sf::VideoMode(WINDOW_W, WINDOW_H), algo.name, sf::Style::Titlebar | sf::Style::Close) {
        window_.setFramerateLimit(60);
        hasFont_ = font_.loadFromFile(FONT_PATH);
        if (!hasFont_) std::cerr << "Font not found (" << FONT_PATH << "), text is not shown.\n";
        setPoints(randomPoints(RANDOM_POINTS, rng_()), "random");
    }

    void run() {
        sf::Clock clock;
        while (window_.isOpen()) {
            sf::Event event;
            while (window_.pollEvent(event)) handle(event);
            update(clock.restart().asSeconds());
            draw();
        }
    }

private:
    // ---- state changes ---------------------------------------------------

    void setPoints(std::vector<Point> points, const std::string& source) {
        points_ = std::move(points);
        source_ = source;
        error_.clear();
        recompute();
        index_ = 0;
        playing_ = false;
    }

    void loadFile(const std::string& name) {
        try {
            setPoints(loadPoints(projectPath("data/" + name + ".csv")), name);
        } catch (const std::exception& e) {
            error_ = e.what();
        }
    }

    void recompute() {
        steps_ = algo_.computeSteps(points_);
        index_ = std::min(index_, lastStep());
        const float r = points_.size() > 200 ? 1.5f : 3.5f;
        pointQuads_.clear();
        for (const Point& p : points_) addDot(pointQuads_, toScreen(p), r, color::point);
    }

    std::size_t lastStep() const { return steps_.empty() ? 0 : steps_.size() - 1; }

    void goTo(long index) {
        index_ = static_cast<std::size_t>(std::clamp<long>(index, 0, static_cast<long>(lastStep())));
    }

    void addPoint(sf::Vector2f pixel) {
        points_.push_back(toWorld(pixel));
        source_ = "custom";
        recompute();
        goTo(static_cast<long>(lastStep()));  // show the updated hull right away
    }

    void removePoint(sf::Vector2f pixel) {
        const auto distance = [&](const Point& p) {
            const sf::Vector2f d = toScreen(p) - pixel;
            return d.x * d.x + d.y * d.y;
        };
        const auto it = std::min_element(points_.begin(), points_.end(),
                                         [&](const Point& a, const Point& b) { return distance(a) < distance(b); });
        if (it == points_.end() || distance(*it) > 10.f * 10.f) return;
        points_.erase(it);
        source_ = "custom";
        recompute();
        goTo(static_cast<long>(lastStep()));
    }

    // ---- input -----------------------------------------------------------

    void handle(const sf::Event& event) {
        if (event.type == sf::Event::Closed) {
            window_.close();
        } else if (event.type == sf::Event::KeyPressed) {
            switch (event.key.code) {
                case sf::Keyboard::Escape: window_.close(); break;
                case sf::Keyboard::Right: playing_ = false; goTo(static_cast<long>(index_) + 1); break;
                case sf::Keyboard::Left: playing_ = false; goTo(static_cast<long>(index_) - 1); break;
                case sf::Keyboard::Home: goTo(0); break;
                case sf::Keyboard::End: goTo(static_cast<long>(lastStep())); break;
                case sf::Keyboard::Up: speed_ = std::min(speed_ + 1, SPEED_COUNT - 1); break;
                case sf::Keyboard::Down: speed_ = std::max(speed_ - 1, 0); break;
                case sf::Keyboard::Space:
                    if (index_ == lastStep()) goTo(0);
                    playing_ = !playing_;
                    progress_ = 0;
                    break;
                case sf::Keyboard::N: setPoints(randomPoints(RANDOM_POINTS, rng_()), "random"); break;
                case sf::Keyboard::C: setPoints({}, "empty"); break;
                case sf::Keyboard::Num1: loadFile("square_1k"); break;
                case sf::Keyboard::Num2: loadFile("disk_1k"); break;
                case sf::Keyboard::Num3: loadFile("circle_1k"); break;
                default: break;
            }
        } else if (event.type == sf::Event::MouseButtonPressed) {
            const sf::Vector2f pixel = window_.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
            if (!insidePlot(pixel)) return;
            if (event.mouseButton.button == sf::Mouse::Left) addPoint(pixel);
            if (event.mouseButton.button == sf::Mouse::Right) removePoint(pixel);
        }
    }

    void update(float seconds) {
        if (!playing_) return;
        progress_ += seconds * SPEEDS[speed_];
        const auto advance = static_cast<long>(progress_);
        progress_ -= static_cast<float>(advance);
        goTo(static_cast<long>(index_) + advance);
        if (index_ == lastStep()) playing_ = false;
    }

    // ---- drawing ---------------------------------------------------------

    void draw() {
        window_.clear(color::background);

        sf::RectangleShape plot({PLOT, PLOT});
        plot.setPosition(MARGIN, MARGIN);
        plot.setFillColor(color::plot);
        plot.setOutlineColor(color::border);
        plot.setOutlineThickness(1.f);
        window_.draw(plot);
        window_.draw(pointQuads_);

        if (!steps_.empty()) drawStep(steps_[index_]);
        drawPanel();
        window_.display();
    }

    void drawStep(const Step& step) {
        // Finished hull: filled polygon.
        if (step.closed && !step.chains.empty() && step.chains[0].size() >= 3) {
            const auto& hull = step.chains[0];
            sf::ConvexShape fill(hull.size());
            for (std::size_t i = 0; i < hull.size(); ++i) fill.setPoint(i, toScreen(hull[i]));
            fill.setFillColor(color::hullFill);
            window_.draw(fill);
        }

        sf::VertexArray quads(sf::Quads);
        for (const auto& chain : step.chains) {
            for (std::size_t i = 0; i + 1 < chain.size(); ++i)
                addLine(quads, toScreen(chain[i]), toScreen(chain[i + 1]), 2.5f, color::hull);
            if (step.closed && chain.size() >= 2)
                addLine(quads, toScreen(chain.back()), toScreen(chain.front()), 2.5f, color::hull);
        }
        for (const auto& s : step.tested) addLine(quads, toScreen(s.a), toScreen(s.b), 2.f, color::tested);
        for (const auto& chain : step.chains)
            for (const Point& p : chain) addDot(quads, toScreen(p), 3.5f, color::hull);
        window_.draw(quads);

        const auto ring = [&](const Point& p, sf::Color c) {
            sf::CircleShape circle(8.f);
            circle.setOrigin(8.f, 8.f);
            circle.setPosition(toScreen(p));
            circle.setFillColor(sf::Color::Transparent);
            circle.setOutlineColor(c);
            circle.setOutlineThickness(2.f);
            window_.draw(circle);
        };
        for (const Point& p : step.current) ring(p, color::current);
        for (const Point& p : step.rejected) ring(p, color::rejected);
    }

    void text(const std::string& str, float x, float y, unsigned size, sf::Color c) {
        if (!hasFont_) return;
        sf::Text t(str, font_, size);
        t.setPosition(x, y);
        t.setFillColor(c);
        window_.draw(t);
    }

    void legend(float y, sf::Color c, const std::string& label) {
        sf::RectangleShape box({14.f, 14.f});
        box.setPosition(PANEL_X, y + 3);
        box.setFillColor(c);
        window_.draw(box);
        text(label, PANEL_X + 24, y, 15, color::text);
    }

    void drawPanel() {
        const float x = PANEL_X;
        text(algo_.name, x, 20, 24, color::text);

        std::ostringstream info;
        info << "Points: " << points_.size() << "   (" << source_ << ")\n"
             << "Step: " << (steps_.empty() ? 0 : index_ + 1) << " / " << steps_.size() << "\n"
             << (playing_ ? "Playing" : "Paused") << ", speed " << SPEEDS[speed_] << " steps/s";
        text(info.str(), x, 62, 16, color::text);

        if (!steps_.empty()) {
            const Step& step = steps_[index_];
            text(wrap(step.description, 46), x, 140, 17, color::current);
            if (step.closed && !step.chains.empty())
                text("Hull points: " + std::to_string(step.chains[0].size()), x, 215, 17, color::hull);
        } else {
            text("No points. Left click to add some.", x, 140, 17, color::dim);
        }

        legend(260, color::hull, "Hull (so far)");
        legend(284, color::tested, "Edges being checked");
        legend(308, color::current, "Current point");
        legend(332, color::rejected, "Point being removed");

        text("Right / Left     next / previous step\n"
             "Space            play / pause\n"
             "Up / Down        faster / slower\n"
             "Home / End       first / last step\n"
             "Left click       add point\n"
             "Right click      remove point\n"
             "N                new random points\n"
             "1 / 2 / 3        square / disk / circle 1k\n"
             "C                clear all points\n"
             "Esc              quit",
             x, 380, 15, color::dim);

        if (!error_.empty()) text(wrap(error_, 46), x, 650, 15, color::rejected);
    }

    const HullAlgorithm& algo_;
    sf::RenderWindow window_;
    sf::Font font_;
    bool hasFont_ = false;
    std::mt19937 rng_{std::random_device{}()};

    std::vector<Point> points_;
    std::string source_;
    std::string error_;
    std::vector<Step> steps_;
    std::size_t index_ = 0;
    sf::VertexArray pointQuads_{sf::Quads};

    bool playing_ = false;
    int speed_ = 3;
    float progress_ = 0;
};

}  // namespace

void runVisualizer(const HullAlgorithm& algo) {
    Visualizer(algo).run();
}
