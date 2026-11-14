#include "dataset.h"
#include "hull_app.h"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <optional>
#include <random>
#include <sstream>

namespace {

constexpr unsigned WINDOW_W = 1200;
constexpr unsigned WINDOW_H = 760;
constexpr float MARGIN = 20.f;
constexpr float PLOT = 720.f;  // the world square [0, WORLD_SIZE]^2 is drawn as PLOT x PLOT pixels
constexpr float PANEL_X = 2 * MARGIN + PLOT;
constexpr std::size_t RANDOM_POINTS = 10;
const char* const FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
const char* const MENU_TITLE = "Convex Hull";
const int SPEEDS[] = {1, 2, 5, 10, 20, 50, 100, 200, 500, 1000};  // steps per second
constexpr int SPEED_COUNT = sizeof(SPEEDS) / sizeof(SPEEDS[0]);

namespace color {
const sf::Color background(24, 26, 33);
const sf::Color plot(32, 35, 44);
const sf::Color border(70, 75, 90);
const sf::Color button(45, 50, 64);
const sf::Color buttonHover(60, 68, 86);
const sf::Color point(150, 160, 180);
const sf::Color hull(80, 200, 140);
const sf::Color hullFill(80, 200, 140, 40);
const sf::Color tested(255, 170, 60);
const sf::Color current(255, 230, 80);
const sf::Color rejected(240, 80, 80);
const sf::Color text(220, 224, 232);
const sf::Color dim(140, 146, 160);
}  // namespace color

// What to do after a screen is left.
enum class Next { Menu, Quit };

// ---- drawing helpers -------------------------------------------------------

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

// SFML 3 has no quads anymore: every rectangle is drawn as two triangles.
void addQuad(sf::VertexArray& triangles, sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Vector2f d,
             sf::Color col) {
    for (const sf::Vector2f p : {a, b, c, a, c, d}) triangles.append(sf::Vertex{p, col});
}

// Thick line (sf::PrimitiveType::Lines is always 1px wide).
void addLine(sf::VertexArray& triangles, sf::Vector2f a, sf::Vector2f b, float width, sf::Color c) {
    const sf::Vector2f d = b - a;
    const float len = d.length();
    if (len < 1e-4f) return;
    const sf::Vector2f n = sf::Vector2f(-d.y, d.x) * (width / 2 / len);
    addQuad(triangles, a + n, b + n, b - n, a - n, c);
}

void addDot(sf::VertexArray& triangles, sf::Vector2f p, float r, sf::Color c) {
    addQuad(triangles, {p.x - r, p.y - r}, {p.x + r, p.y - r}, {p.x + r, p.y + r}, {p.x - r, p.y + r}, c);
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

// font == nullptr: the font could not be loaded, text is skipped.
void drawText(sf::RenderWindow& window, const sf::Font* font, const std::string& str, sf::Vector2f pos,
              unsigned size, sf::Color c, bool centered = false) {
    if (!font) return;
    sf::Text t(*font, str, size);
    t.setFillColor(c);
    if (centered) {
        const sf::FloatRect bounds = t.getLocalBounds();
        t.setOrigin(bounds.position + bounds.size / 2.f);
    }
    t.setPosition(pos);
    window.draw(t);
}

sf::Vector2f mousePosition(const sf::RenderWindow& window) {
    return window.mapPixelToCoords(sf::Mouse::getPosition(window));
}

bool isLeftClick(const sf::Event& event, const sf::RenderWindow& window, const sf::FloatRect& area) {
    const auto* mouse = event.getIf<sf::Event::MouseButtonPressed>();
    return mouse && mouse->button == sf::Mouse::Button::Left &&
           area.contains(window.mapPixelToCoords(mouse->position));
}

bool isKey(const sf::Event& event, sf::Keyboard::Key code) {
    const auto* key = event.getIf<sf::Event::KeyPressed>();
    return key && key->code == code;
}

struct Button {
    sf::FloatRect rect;
    std::string label;
    unsigned textSize = 20;

    bool clicked(const sf::Event& event, const sf::RenderWindow& window) const {
        return isLeftClick(event, window, rect);
    }

    void draw(sf::RenderWindow& window, const sf::Font* font) const {
        const bool hover = rect.contains(mousePosition(window));
        sf::RectangleShape box(rect.size);
        box.setPosition(rect.position);
        box.setFillColor(hover ? color::buttonHover : color::button);
        box.setOutlineColor(hover ? color::hull : color::border);
        box.setOutlineThickness(2.f);
        window.draw(box);
        drawText(window, font, label, rect.position + rect.size / 2.f, textSize, color::text, true);
    }
};

// ---- visualization of one algorithm ---------------------------------------

class AlgorithmView {
public:
    AlgorithmView(sf::RenderWindow& window, const sf::Font* font, const HullAlgorithm& algo)
        : algo_(algo), window_(window), font_(font) {
        window_.setTitle(algo.name);
        setPoints(randomPoints(RANDOM_POINTS, rng_()), "random");
    }

    Next run() {
        sf::Clock clock;
        while (true) {
            while (const std::optional event = window_.pollEvent())
                if (const std::optional next = handle(*event)) return *next;
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
        pointDots_.clear();
        for (const Point& p : points_) addDot(pointDots_, toScreen(p), r, color::point);
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
        const auto distance = [&](const Point& p) { return (toScreen(p) - pixel).lengthSquared(); };
        const auto it = std::min_element(points_.begin(), points_.end(),
                                         [&](const Point& a, const Point& b) { return distance(a) < distance(b); });
        if (it == points_.end() || distance(*it) > 10.f * 10.f) return;
        points_.erase(it);
        source_ = "custom";
        recompute();
        goTo(static_cast<long>(lastStep()));
    }

    // ---- input -----------------------------------------------------------

    std::optional<Next> handle(const sf::Event& event) {
        if (event.is<sf::Event::Closed>()) return Next::Quit;
        if (back_.clicked(event, window_)) return Next::Menu;

        if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
            using Key = sf::Keyboard::Key;
            switch (key->code) {
                case Key::Escape:
                case Key::Backspace: return Next::Menu;
                case Key::Right: playing_ = false; goTo(static_cast<long>(index_) + 1); break;
                case Key::Left: playing_ = false; goTo(static_cast<long>(index_) - 1); break;
                case Key::Home: goTo(0); break;
                case Key::End: goTo(static_cast<long>(lastStep())); break;
                case Key::Up: speed_ = std::min(speed_ + 1, SPEED_COUNT - 1); break;
                case Key::Down: speed_ = std::max(speed_ - 1, 0); break;
                case Key::Space:
                    if (index_ == lastStep()) goTo(0);
                    playing_ = !playing_;
                    progress_ = 0;
                    break;
                case Key::N: setPoints(randomPoints(RANDOM_POINTS, rng_()), "random"); break;
                case Key::C: setPoints({}, "empty"); break;
                case Key::Num1: loadFile("square_1k"); break;
                case Key::Num2: loadFile("disk_1k"); break;
                case Key::Num3: loadFile("circle_1k"); break;
                default: break;
            }
        } else if (const auto* mouse = event.getIf<sf::Event::MouseButtonPressed>()) {
            const sf::Vector2f pixel = window_.mapPixelToCoords(mouse->position);
            if (insidePlot(pixel)) {
                if (mouse->button == sf::Mouse::Button::Left) addPoint(pixel);
                if (mouse->button == sf::Mouse::Button::Right) removePoint(pixel);
            }
        }
        return std::nullopt;
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
        plot.setPosition({MARGIN, MARGIN});
        plot.setFillColor(color::plot);
        plot.setOutlineColor(color::border);
        plot.setOutlineThickness(1.f);
        window_.draw(plot);
        window_.draw(pointDots_);

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

        sf::VertexArray triangles(sf::PrimitiveType::Triangles);
        for (const auto& chain : step.chains) {
            for (std::size_t i = 0; i + 1 < chain.size(); ++i)
                addLine(triangles, toScreen(chain[i]), toScreen(chain[i + 1]), 2.5f, color::hull);
            if (step.closed && chain.size() >= 2)
                addLine(triangles, toScreen(chain.back()), toScreen(chain.front()), 2.5f, color::hull);
        }
        for (const auto& s : step.tested) addLine(triangles, toScreen(s.a), toScreen(s.b), 2.f, color::tested);
        for (const auto& chain : step.chains)
            for (const Point& p : chain) addDot(triangles, toScreen(p), 3.5f, color::hull);
        window_.draw(triangles);

        const auto ring = [&](const Point& p, sf::Color c) {
            sf::CircleShape circle(8.f);
            circle.setOrigin({8.f, 8.f});
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
        drawText(window_, font_, str, {x, y}, size, c);
    }

    void legend(float y, sf::Color c, const std::string& label) {
        sf::RectangleShape box({14.f, 14.f});
        box.setPosition({PANEL_X, y + 3});
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

        text("Right / Left\nSpace\nUp / Down\nHome / End\nLeft click\nRight click\nN\n1 / 2 / 3\nC\nEsc", x, 380, 15,
             color::dim);
        text("next / previous step\n"
             "play / pause\n"
             "faster / slower\n"
             "first / last step\n"
             "add point\n"
             "remove point\n"
             "new random points\n"
             "square / disk / circle 1k\n"
             "clear all points\n"
             "back to the menu",
             x + 120, 380, 15, color::dim);

        if (!error_.empty()) text(wrap(error_, 46), x, 600, 15, color::rejected);
        back_.draw(window_, font_);
    }

    const HullAlgorithm& algo_;
    sf::RenderWindow& window_;
    const sf::Font* font_;
    std::mt19937 rng_{std::random_device{}()};
    Button back_{{{PANEL_X, 690.f}, {200.f, 44.f}}, "< Algorithms"};

    std::vector<Point> points_;
    std::string source_;
    std::string error_;
    std::vector<Step> steps_;
    std::size_t index_ = 0;
    sf::VertexArray pointDots_{sf::PrimitiveType::Triangles};

    bool playing_ = false;
    int speed_ = 3;
    float progress_ = 0;
};

// ---- placeholder for algorithms that are not implemented yet ---------------

Next runTodo(sf::RenderWindow& window, const sf::Font* font, const HullAlgorithm& algo) {
    window.setTitle(algo.name);
    const Button back{{{(WINDOW_W - 200.f) / 2, 460.f}, {200.f, 44.f}}, "< Algorithms"};
    const sf::Vector2f center(WINDOW_W / 2.f, 0.f);

    while (true) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) return Next::Quit;
            if (isKey(*event, sf::Keyboard::Key::Escape) || isKey(*event, sf::Keyboard::Key::Backspace) ||
                back.clicked(*event, window))
                return Next::Menu;
        }
        window.clear(color::background);
        drawText(window, font, algo.name, center + sf::Vector2f(0, 200), 32, color::text, true);
        drawText(window, font, "TODO", center + sf::Vector2f(0, 330), 64, color::tested, true);
        back.draw(window, font);
        window.display();
    }
}

}  // namespace

// ---- menu ------------------------------------------------------------------

void runVisualizer(const std::vector<HullAlgorithm>& algos) {
    sf::RenderWindow window(sf::VideoMode({WINDOW_W, WINDOW_H}), MENU_TITLE, sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    sf::Font fontStorage;
    const sf::Font* font = fontStorage.openFromFile(FONT_PATH) ? &fontStorage : nullptr;
    if (!font) std::cerr << "Font not found (" << FONT_PATH << "), text is not shown.\n";

    std::vector<Button> buttons;
    for (std::size_t i = 0; i < algos.size(); ++i) {
        const float y = 280.f + static_cast<float>(i) * 120.f;
        buttons.push_back({{{(WINDOW_W - 420.f) / 2, y}, {420.f, 90.f}}, algos[i].name, 24});
    }

    const auto open = [&](const HullAlgorithm& algo) {
        const Next next = algo.implemented() ? AlgorithmView(window, font, algo).run() : runTodo(window, font, algo);
        window.setTitle(MENU_TITLE);
        if (next == Next::Quit) window.close();
    };

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>() || isKey(*event, sf::Keyboard::Key::Escape)) {
                window.close();
            } else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                // Number keys 1..n open the n-th algorithm.
                const int i = static_cast<int>(key->code) - static_cast<int>(sf::Keyboard::Key::Num1);
                if (i >= 0 && i < static_cast<int>(algos.size())) open(algos[i]);
            } else {
                for (std::size_t i = 0; i < buttons.size(); ++i)
                    if (buttons[i].clicked(*event, window)) open(algos[i]);
            }
        }
        if (!window.isOpen()) break;

        const sf::Vector2f center(WINDOW_W / 2.f, 0.f);
        window.clear(color::background);
        drawText(window, font, MENU_TITLE, center + sf::Vector2f(0, 150), 40, color::text, true);
        drawText(window, font, "Choose an algorithm", center + sf::Vector2f(0, 210), 20, color::dim, true);
        for (const Button& b : buttons) b.draw(window, font);
        drawText(window, font, "Click or press 1-" + std::to_string(algos.size()) + "   |   Esc: quit",
                 center + sf::Vector2f(0, WINDOW_H - 60.f), 16, color::dim, true);
        window.display();
    }
}
