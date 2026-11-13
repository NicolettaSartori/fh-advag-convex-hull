#include <SFML/Graphics.hpp>

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "Hallo SFML!");
    window.setFramerateLimit(60);

    sf::CircleShape ball(40.f);
    ball.setFillColor(sf::Color(0, 200, 120));
    ball.setPosition(100.f, 100.f);
    sf::Vector2f velocity(4.f, 3.f);

    sf::Font font;
    font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
    sf::Text text("Beliebige Taste: Farbe | ESC: Beenden", font, 22);
    text.setPosition(20.f, 20.f);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Escape)
                    window.close();
                else
                    ball.setFillColor(sf::Color(rand() % 256, rand() % 256, rand() % 256));
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
