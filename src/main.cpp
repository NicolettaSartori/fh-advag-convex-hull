#include <SFML/Graphics.hpp>

#include <cstdint>
#include <cstdlib>
#include <optional>

int main()
{
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Hallo SFML!");
    window.setFramerateLimit(60);

    sf::CircleShape ball(40.f);
    ball.setFillColor(sf::Color(0, 200, 120));
    ball.setPosition({100.f, 100.f});
    sf::Vector2f velocity(4.f, 3.f);

    const sf::Font font("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
    sf::Text text(font, "Beliebige Taste: Farbe | ESC: Beenden", 22);
    text.setPosition({20.f, 20.f});

    const auto randomByte = [] { return static_cast<std::uint8_t>(std::rand() % 256); };

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
            else if (const auto* key = event->getIf<sf::Event::KeyPressed>())
            {
                if (key->code == sf::Keyboard::Key::Escape)
                    window.close();
                else
                    ball.setFillColor(sf::Color(randomByte(), randomByte(), randomByte()));
            }
        }

        // Ball bewegen und an den Rändern abprallen lassen
        ball.move(velocity);
        sf::Vector2f pos = ball.getPosition();
        float d = ball.getRadius() * 2.f;
        if (pos.x < 0 || pos.x + d > window.getSize().x) velocity.x = -velocity.x;
        if (pos.y < 0 || pos.y + d > window.getSize().y) velocity.y = -velocity.y;

        window.clear(sf::Color(30, 30, 40));
        window.draw(ball);
        window.draw(text);
        window.display();
    }
    return 0;
}
