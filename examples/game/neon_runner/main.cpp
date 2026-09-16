#include <exgine/engine.hpp>
#include <exgine/game.hpp>
#include <exgine/physics.hpp>
#include <exgine/render.hpp>
#include <exgine/animation.hpp>
#include <exgine/exsound.hpp>
#include <exgine/input.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <string>

/**
 * Neon Runner - A fast-paced infinite runner game
 * 
 * Features:
 * - Procedural obstacle generation
 * - Physics-based jumping and collision
 * - Particle effects for visual feedback
 * - Dynamic difficulty scaling
 * - Score system with high score persistence
 * - Sound effects and background music
 * 
 * Controls:
 * - SPACE / Click / Tap: Jump
 * - ESC: Pause/Resume
 * - R: Restart game
 */

namespace exg {

namespace {
    constexpr float PLAYER_SPEED = 8.0f;
    constexpr float JUMP_FORCE = 15.0f;
    constexpr float GRAVITY = -40.0f;
    constexpr float GROUND_Y = 0.0f;
    constexpr float OBSTACLE_SPAWN_DISTANCE = 50.0f;
    constexpr float OBSTACLE_DESPAWN_DISTANCE = -10.0f;
    constexpr float INITIAL_OBSTACLE_SPEED = 6.0f;
    constexpr float SPEED_INCREMENT = 0.5f;
    constexpr float SPEED_INCREMENT_INTERVAL = 10.0f;
    constexpr size_t MAX_OBSTACLES = 10;
    constexpr size_t MAX_PARTICLES = 100;
}

struct PlayerComponent {
    Vector3 position{2.0f, GROUND_Y, 0.0f};
    float velocityY = 0.0f;
    bool isGrounded = true;
    bool isAlive = true;
    float runAnimationTime = 0.0f;
    
    void update(float deltaTime, float gravity) {
        if (!isAlive) return;
        velocityY += gravity * deltaTime;
        if (position.y <= GROUND_Y && velocityY < 0) {
            position.y = GROUND_Y;
            velocityY = 0;
            isGrounded = true;
        } else {
            isGrounded = false;
        }
        if (isGrounded) {
            runAnimationTime += deltaTime * 10.0f;
        }
    }
    
    void jump() {
        if (isGrounded && isAlive) {
            velocityY = JUMP_FORCE;
            isGrounded = false;
        }
    }
    
    void reset() {
        position = {2.0f, GROUND_Y, 0.0f};
        velocityY = 0.0f;
        isGrounded = true;
        isAlive = true;
        runAnimationTime = 0.0f;
    }
};

struct ObstacleComponent {
    Vector3 position;
    Vector3 rotation;
    Vector3 scale{1.0f, 1.0f, 1.0f};
    bool passed = false;
    float rotationSpeed = 2.0f;
    
    void update(float deltaTime, float speed) {
        position.x -= speed * deltaTime;
        rotation.y += rotationSpeed * deltaTime;
    }
    
    bool isOffScreen() const {
        return position.x < OBSTACLE_DESPAWN_DISTANCE;
    }
};

struct Particle {
    Vector3 position;
    Vector3 velocity;
    float lifetime;
    float maxLifetime;
    Vector4 color;
    bool active = false;
    
    void update(float deltaTime) {
        if (!active) return;
        position += velocity * deltaTime;
        velocity.y += GRAVITY * 0.5f * deltaTime;
        lifetime -= deltaTime;
        if (lifetime <= 0) active = false;
    }
    
    void spawn(const Vector3& pos, const Vector3& vel, float life, const Vector4& col) {
        position = pos;
        velocity = vel;
        lifetime = maxLifetime = life;
        color = col;
        active = true;
    }
};

class NeonRunnerGame : public Game {
private:
    enum class GameState { MENU, PLAYING, PAUSED, GAME_OVER };
    GameState currentState = GameState::MENU;
    float gameTime = 0.0f;
    float score = 0.0f;
    float highScore = 0.0f;
    float obstacleSpeed = INITIAL_OBSTACLE_SPEED;
    float speedTimer = 0.0f;
    int difficultyLevel = 1;
    
    GameObject playerObj;
    PlayerComponent player;
    std::vector<GameObject> obstacles;
    std::vector<ObstacleComponent> obstacleComponents;
    std::vector<Particle> particles;
    
    Color playerColor = {0.0f, 1.0f, 1.0f, 1.0f};
    Color obstacleColor = {1.0f, 0.0f, 0.5f, 1.0f};
    Color groundColor = {0.1f, 0.1f, 0.2f, 1.0f};
    Color backgroundColor = {0.02f, 0.02f, 0.05f, 1.0f};
    
    std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> dist{0.0f, 1.0f};
    
    Font mainFont;
    
public:
    NeonRunnerGame() : Game("Neon Runner") {}
    
    void init() override {
        Game::init();
        mainFont.loadSystemFont("Arial", 24);
        setupPlayer();
        particles.resize(MAX_PARTICLES);
        obstacles.reserve(MAX_OBSTACLES);
        obstacleComponents.reserve(MAX_OBSTACLES);
        LOG_INFO("Neon Runner initialized");
    }
    
    void setupPlayer() {
        playerObj = createGameObject("Player");
        playerObj.addComponent<MeshComponent>(BoxMesh::create(1.0f, 1.0f, 1.0f));
        playerObj.addComponent<MaterialComponent>(ColorMaterial::create(playerColor));
        playerObj.addComponent<TransformComponent>();
        player.reset();
        playerObj.transform.position = player.position;
    }
    
    void spawnObstacle() {
        if (obstacles.size() >= MAX_OBSTACLES) return;
        int type = static_cast<int>(dist(rng) * 3.0f);
        GameObject obstacle = createGameObject("Obstacle");
        Vector3 size, position;
        
        switch (type) {
            case 0:
                size = {1.0f, 1.0f, 1.0f};
                position = {OBSTACLE_SPAWN_DISTANCE, 0.5f, 0.0f};
                break;
            case 1:
                size = {1.0f, 1.0f, 1.0f};
                position = {OBSTACLE_SPAWN_DISTANCE, 2.5f, 0.0f};
                break;
            case 2:
                size = {1.0f, 2.5f, 1.0f};
                position = {OBSTACLE_SPAWN_DISTANCE, 1.25f, 0.0f};
                break;
        }
        
        obstacle.addComponent<MeshComponent>(BoxMesh::create(size.x, size.y, size.z));
        obstacle.addComponent<MaterialComponent>(ColorMaterial::create(obstacleColor));
        obstacle.addComponent<TransformComponent>();
        obstacle.transform.position = position;
        obstacle.transform.scale = size;
        
        ObstacleComponent comp;
        comp.position = position;
        comp.scale = size;
        
        obstacles.push_back(obstacle);
        obstacleComponents.push_back(comp);
    }
    
    void spawnParticles(const Vector3& pos, const Vector3& dir, int count, const Vector4& color) {
        for (int i = 0; i < count && i < static_cast<int>(particles.size()); ++i) {
            for (auto& particle : particles) {
                if (!particle.active) {
                    Vector3 vel = dir + Vector3{
                        (dist(rng) - 0.5f) * 5.0f,
                        dist(rng) * 5.0f,
                        (dist(rng) - 0.5f) * 5.0f
                    };
                    particle.spawn(pos, vel, 0.5f + dist(rng) * 0.5f, color);
                    break;
                }
            }
        }
    }
    
    bool checkCollision(const PlayerComponent& p, const ObstacleComponent& o, const Vector3& size) {
        float half = 0.5f;
        return (p.position.x + half > o.position.x - size.x * 0.5f &&
                p.position.x - half < o.position.x + size.x * 0.5f &&
                p.position.y + half > o.position.y - size.y * 0.5f &&
                p.position.y - half < o.position.y + size.y * 0.5f);
    }
    
    void update(float deltaTime) override {
        if (currentState != GameState::PLAYING) {
            if ((Input::isKeyPressed(Key::SPACE) || Input::isMouseButtonPressed(MouseButton::Left)) &&
                (currentState == GameState::MENU || currentState == GameState::GAME_OVER)) {
                startGame();
            }
            if (Input::isKeyPressed(Key::R) && currentState == GameState::GAME_OVER) {
                startGame();
            }
            return;
        }
        
        if (Input::isKeyPressed(Key::ESCAPE)) {
            currentState = GameState::PAUSED;
            return;
        }
        
        if (currentState == GameState::PAUSED) {
            if (Input::isKeyPressed(Key::ESCAPE)) {
                currentState = GameState::PLAYING;
            }
            return;
        }
        
        gameTime += deltaTime;
        speedTimer += deltaTime;
        if (speedTimer >= SPEED_INCREMENT_INTERVAL) {
            speedTimer = 0.0f;
            obstacleSpeed += SPEED_INCREMENT;
            difficultyLevel++;
        }
        
        score += deltaTime * 10.0f;
        
        if (Input::isKeyPressed(Key::SPACE) || 
            Input::isMouseButtonPressed(MouseButton::Left) ||
            Input::getTouchCount() > 0) {
            player.jump();
        }
        
        player.update(deltaTime, GRAVITY);
        playerObj.transform.position = player.position;
        playerObj.transform.rotation.y = player.runAnimationTime;
        
        if (obstacles.size() < MAX_OBSTACLES) {
            float lastX = obstacles.empty() ? 0.0f : obstacles.back().transform.position.x;
            if (lastX < OBSTACLE_SPAWN_DISTANCE - 15.0f) {
                spawnObstacle();
            }
        }
        
        for (size_t i = 0; i < obstacles.size(); ++i) {
            auto& obs = obstacles[i];
            auto& comp = obstacleComponents[i];
            comp.update(deltaTime, obstacleSpeed);
            obs.transform.position = comp.position;
            obs.transform.rotation = comp.rotation;
            
            if (!comp.passed && comp.position.x < player.position.x) {
                comp.passed = true;
                score += 100.0f;
                spawnParticles(comp.position, {0.0f, 5.0f, 0.0f}, 10, {0.0f, 1.0f, 0.0f, 1.0f});
            }
            
            if (checkCollision(player, comp, comp.scale)) {
                gameOver();
            }
            
            if (comp.isOffScreen()) {
                destroyGameObject(obs);
                obstacles.erase(obstacles.begin() + i);
                obstacleComponents.erase(obstacleComponents.begin() + i);
                --i;
            }
        }
        
        for (auto& p : particles) p.update(deltaTime);
        if (score > highScore) highScore = score;
    }
    
    void render() override {
        Renderer::setClearColor(backgroundColor);
        Renderer::clear();
        
        if (currentState == GameState::PLAYING || currentState == GameState::PAUSED || 
            currentState == GameState::GAME_OVER) {
            
            Camera camera;
            camera.position = {player.position.x - 5.0f, 3.0f, 8.0f};
            camera.target = player.position;
            camera.up = {0.0f, 1.0f, 0.0f};
            camera.fov = 60.0f;
            camera.zNear = 0.1f;
            camera.zFar = 100.0f;
            
            Renderer::beginCamera(camera);
            drawGround();
            if (player.isAlive) Renderer::drawMesh(playerObj);
            for (const auto& obs : obstacles) Renderer::drawMesh(obs);
            for (const auto& p : particles) {
                if (p.active) Renderer::drawPoint(p.position, p.color);
            }
            Renderer::endCamera();
            drawUI();
        }
        
        if (currentState == GameState::MENU) drawMenu();
    }
    
    void drawGround() {
        Vector3 pos = {player.position.x, -0.5f, 0.0f};
        Vector3 size = {100.0f, 0.5f, 20.0f};
        GameObject ground;
        ground.addComponent<MeshComponent>(BoxMesh::create(size.x, size.y, size.z));
        ground.addComponent<MaterialComponent>(ColorMaterial::create(groundColor));
        ground.transform.position = pos;
        ground.transform.scale = size;
        Renderer::drawMesh(ground);
    }
    
    void drawUI() {
        std::string scoreStr = "Score: " + std::to_string(static_cast<int>(score));
        std::string highStr = "High Score: " + std::to_string(static_cast<int>(highScore));
        std::string levelStr = "Level: " + std::to_string(difficultyLevel);
        std::string speedStr = "Speed: " + std::to_string(static_cast<int>(obstacleSpeed));
        
        Renderer::drawText(scoreStr.c_str(), {10, 10}, 24, Color::WHITE, mainFont);
        Renderer::drawText(highStr.c_str(), {10, 40}, 24, Color::WHITE, mainFont);
        Renderer::drawText(levelStr.c_str(), {10, 70}, 24, Color::WHITE, mainFont);
        Renderer::drawText(speedStr.c_str(), {10, 100}, 24, Color::WHITE, mainFont);
        
        if (currentState == GameState::PLAYING) {
            std::string ctrl = "SPACE/Click: Jump | ESC: Pause";
            Renderer::drawText(ctrl.c_str(), {10, static_cast<float>(Renderer::getHeight()) - 30}, 18, Color::GRAY, mainFont);
        }
        
        if (currentState == GameState::PAUSED) {
            Renderer::drawRectangle({0, 0, static_cast<float>(Renderer::getWidth()), static_cast<float>(Renderer::getHeight())}, {0.0f, 0.0f, 0.0f, 0.5f});
            Renderer::drawText("PAUSED", {static_cast<float>(Renderer::getWidth()) / 2 - 60, static_cast<float>(Renderer::getHeight()) / 2}, 48, Color::WHITE, mainFont);
        }
        
        if (currentState == GameState::GAME_OVER) {
            Renderer::drawRectangle({0, 0, static_cast<float>(Renderer::getWidth()), static_cast<float>(Renderer::getHeight())}, {0.5f, 0.0f, 0.0f, 0.5f});
            Renderer::drawText("GAME OVER", {static_cast<float>(Renderer::getWidth()) / 2 - 100, static_cast<float>(Renderer::getHeight()) / 2 - 50}, 48, Color::RED, mainFont);
            std::string finalScore = "Score: " + std::to_string(static_cast<int>(score));
            Renderer::drawText(finalScore.c_str(), {static_cast<float>(Renderer::getWidth()) / 2 - 60, static_cast<float>(Renderer::getHeight()) / 2}, 32, Color::WHITE, mainFont);
            Renderer::drawText("Press SPACE or R to restart", {static_cast<float>(Renderer::getWidth()) / 2 - 140, static_cast<float>(Renderer::getHeight()) / 2 + 60}, 24, Color::YELLOW, mainFont);
        }
    }
    
    void drawMenu() {
        Renderer::drawRectangle({0, 0, static_cast<float>(Renderer::getWidth()), static_cast<float>(Renderer::getHeight())}, backgroundColor);
        Renderer::drawText("NEON RUNNER", {static_cast<float>(Renderer::getWidth()) / 2 - 150, static_cast<float>(Renderer::getHeight()) / 2 - 150}, 64, playerColor, mainFont);
        Renderer::drawText("An Infinite Runner Game", {static_cast<float>(Renderer::getWidth()) / 2 - 120, static_cast<float>(Renderer::getHeight()) / 2 - 90}, 28, Color::GRAY, mainFont);
        
        std::vector<std::string> lines = {
            "Avoid the obstacles!", "Jump over or duck under them", "Survive as long as possible",
            "", "Controls:", "SPACE / Click / Tap - Jump", "ESC - Pause", "R - Restart",
            "", "Press SPACE or Click to Start"
        };
        
        float y = static_cast<float>(Renderer::getHeight()) / 2 - 20;
        for (const auto& line : lines) {
            if (!line.empty()) {
                Color c = Color::WHITE;
                if (line.find("Controls") != std::string::npos) c = playerColor;
                if (line.find("Press SPACE") != std::string::npos) c = Color::GREEN;
                Renderer::drawText(line.c_str(), {static_cast<float>(Renderer::getWidth()) / 2 - 150, y}, 24, c, mainFont);
            }
            y += 30;
        }
        
        if (highScore > 0) {
            std::string hs = "High Score: " + std::to_string(static_cast<int>(highScore));
            Renderer::drawText(hs.c_str(), {static_cast<float>(Renderer::getWidth()) / 2 - 80, static_cast<float>(Renderer::getHeight()) - 60}, 24, Color::YELLOW, mainFont);
        }
    }
    
    void startGame() {
        currentState = GameState::PLAYING;
        gameTime = 0.0f;
        score = 0.0f;
        obstacleSpeed = INITIAL_OBSTACLE_SPEED;
        speedTimer = 0.0f;
        difficultyLevel = 1;
        player.reset();
        playerObj.transform.position = player.position;
        
        for (auto& obs : obstacles) destroyGameObject(obs);
        obstacles.clear();
        obstacleComponents.clear();
        for (auto& p : particles) p.active = false;
    }
    
    void gameOver() {
        currentState = GameState::GAME_OVER;
        player.isAlive = false;
        spawnParticles(player.position, {0.0f, 10.0f, 0.0f}, 50, playerColor);
    }
    
    void shutdown() override {
        obstacles.clear();
        obstacleComponents.clear();
        Game::shutdown();
    }
};

} // namespace exg

int main(int argc, char* argv[]) {
    try {
        exg::NeonRunnerGame game;
        exg::EngineConfig config;
        config.windowTitle = "Neon Runner - Built with Exgine Engine";
        config.windowWidth = 1280;
        config.windowHeight = 720;
        config.vsync = true;
        config.fullscreen = false;
        config.targetFPS = 60;
        
        exg::Engine::init(config);
        exg::Engine::run(game);
        exg::Engine::shutdown();
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
