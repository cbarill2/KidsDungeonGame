#include "Dungeon.h"

void Dungeon::draw(sf::RenderWindow *window)
{
    for (const auto &indexSpeedPair : m_movableTiles)
    {
        m_tiles[indexSpeedPair.first].setFillColor(sf::Color::Cyan);
    }

    for (int i = 0; i < m_numberOfTiles; ++i)
    {
        window->draw(m_tiles[i]);
    }

    for (auto &&indexEnemyPair : m_enemies)
    {
        window->draw(indexEnemyPair.second);
    }
    

    for (const auto &indexSpeedPair : m_movableTiles)
    {
        m_tiles[indexSpeedPair.first].setFillColor(sf::Color::White);
    }
}

void Dungeon::buildMovableTilesMap(sf::Vector2f playerPosition, int playerSpeed)
{
    int tileIndex;

    if (isValidTile(playerPosition, tileIndex) && !m_tiles[tileIndex].hasCollision())
    {
        if (playerSpeed > 0)
        {
            buildMovableTilesMap(sf::Vector2f{playerPosition.x, playerPosition.y + c_tileHeightf}, playerSpeed - 1);
            buildMovableTilesMap(sf::Vector2f{playerPosition.x - c_tileWidthf, playerPosition.y}, playerSpeed - 1);
            buildMovableTilesMap(sf::Vector2f{playerPosition.x + c_tileWidthf, playerPosition.y}, playerSpeed - 1);
            buildMovableTilesMap(sf::Vector2f{playerPosition.x, playerPosition.y - c_tileHeightf}, playerSpeed - 1);
        }
        if (!m_tiles[tileIndex].hasUnit())
        {
            auto search = m_movableTiles.find(tileIndex);
            if (search == m_movableTiles.end() || playerSpeed > search->second)
            {
                m_movableTiles[tileIndex] = playerSpeed;
            }
        }
    }
}

void Dungeon::clearMovableTiles()
{
    m_movableTiles.clear();
}

bool Dungeon::isMovableTile(int tileIndex, int &speedLeft)
{
    auto search = m_movableTiles.find(tileIndex);
    bool found = (search != m_movableTiles.end());
    speedLeft = (found) ? search->second : 0;
    return found;
}

bool Dungeon::isAttackableTile(int tileIndex)
{
    return std::find(m_attackableTiles.begin(), m_attackableTiles.end(), tileIndex) != m_attackableTiles.end();
}

void Dungeon::buildAttackableTilesMap(sf::Vector2f playerPosition, int minRange, int maxRange)
{
    clearAttackableTiles();

    if (maxRange == 0)
        return; //maxRange 0 means targeting self?
    int tileIndex;

    // for each tile in range, is it: valid, has a live enemy, and in LOS
    for (int i = playerPosition.x - maxRange; i <= playerPosition.x + maxRange; i += c_tileWidthi)
    {
        for (int j = playerPosition.y - maxRange; j <= playerPosition.y + maxRange; j += c_tileHeighti)
        {
            int dist = abs(playerPosition.x - i) + abs(playerPosition.y - j);
            if (dist <= maxRange && dist >= minRange && isValidTile(sf::Vector2f{(float)i, (float)j}, tileIndex) && m_tiles[tileIndex].hasUnit())
            {
                auto search = m_enemies.find(tileIndex);
                if (search != m_enemies.end() && search->second.isAlive() &&
                    los(playerPosition, sf::Vector2f{(float)i, (float)j}))
                {
                    m_attackableTiles.push_back(tileIndex);
                    m_tiles[tileIndex].setFillColor(sf::Color::Red);
                }
            }
        }
    }
}

void Dungeon::clearAttackableTiles()
{
    for (auto &&tile : m_attackableTiles)
    {
        m_tiles[tile].setFillColor(sf::Color::White);
    }
    
    m_attackableTiles.clear();    
}

bool Dungeon::isTileAtPosition(sf::Vector2f &position)
{
    int tileIndex;
    if (isValidTile(position, tileIndex))
    {
        position = m_tiles[tileIndex].getPosition();
        return true;
    }
    return false;
}

bool Dungeon::tileHasUnit(sf::Vector2f position)
{
    int tileIndex;
    if (isValidTile(position, tileIndex))
    {
        return m_tiles[tileIndex].hasUnit();
    }
    return false;
}

Tile &Dungeon::getTileAtPosition(sf::Vector2f position)
{
    int tileIndex = ((int)(position.y / c_tileHeightf) * m_width) + (int)(position.x / c_tileWidthf);
    return m_tiles[tileIndex];
}

bool Dungeon::isValidTile(sf::Vector2f position, int &tileIndex)
{
    if (position.x < 0 || position.x >= m_width * c_tileWidthf || position.y < 0 || position.y >= m_height * c_tileHeightf)
    {
        return false;
    }
    tileIndex = ((int)(position.y / c_tileHeightf) * m_width) + (int)(position.x / c_tileWidthf);
    return (tileIndex >= 0 && tileIndex < m_numberOfTiles);
}

void Dungeon::removeEnemy(int defeatedEnemyIndex)
{
    m_enemies.erase(defeatedEnemyIndex);
    m_tiles[defeatedEnemyIndex].toggleUnit();
}

void Dungeon::reset()
{
    delete[] m_tiles;
    m_tiles = new Tile[m_numberOfTiles];
    m_enemies.clear();

    generateProcedurally();
    populateWithEnemies();
}

void Dungeon::generateProcedurally()
{
    int *heights{new int[m_width]{0}};
    sf::Vector2f tileSize{c_tileWidthf, c_tileHeightf};
    PRNG prng{};
    prng.seed64(1);
    prng.seed128(prng.nextSplitMix64(), prng.nextSplitMix64());
    int roomWidth, roomHeight, tileTextureIndex;
    int currentWidth{0}, currentHeight{0}, minHeight{m_height};
    while (currentHeight < m_height - 1)
    {
        currentWidth = 0;
        while (currentWidth < m_width - 1)
        {
            currentHeight = heights[currentWidth + 1];
            roomHeight = prng.random_roll(5, 6);
            roomWidth = prng.random_roll(5, 6);

            int yEdge = currentHeight + roomHeight;
            int xEdge = currentWidth + roomWidth;
            if (yEdge > m_height)
            {
                yEdge = m_height;
            }
            if (xEdge > m_width)
            {
                xEdge = m_width;
            }

            minHeight = currentHeight;
            for (int x = currentWidth; x < xEdge; ++x)
            {
                if (heights[x] < minHeight)
                {
                    minHeight = heights[x];
                }
            }

            int numberOfDoors = prng.random_roll(4);

            for (int y = minHeight; y < yEdge; ++y)
            {
                for (int x = currentWidth; x < xEdge; ++x)
                {
                    int tileIndex = (y * m_width) + x;

                    if ((x == currentWidth && y == minHeight)     //top left corner
                        || (x == currentWidth && y == yEdge - 1)  //bottom left corner
                        || (x == xEdge - 1 && y == minHeight)     //top right corner
                        || (x == xEdge - 1 && y == yEdge - 1)     //bottom right corner
                        || (y == minHeight || y == yEdge - 1)     //wall going from left to right
                        || (x == currentWidth || x == xEdge - 1)) //wall going from top to bottom
                    {
                        if (y > 0 && x > 0 && y != m_height - 1 && x != m_width - 1             // no doors on outer walls
                            && numberOfDoors > 0                                                //we have more doors to place
                            && ((xEdge - x) == roomWidth / 2 || (yEdge - y) == roomHeight / 2)) //try to center doors
                        {
                            // door tile
                            tileTextureIndex = 2;
                            --numberOfDoors;
                        }
                        else if (x == 1 && y == 0)
                        {
                            // dungeon entrance door
                            tileTextureIndex = 2;
                        }
                        else
                        {
                            // wall tile
                            tileTextureIndex = 9;
                        }
                    }
                    else
                    {
                        //floor tile
                        tileTextureIndex = 0;
                    }

                    m_tiles[tileIndex] = Tile{tileSize, tileTextureIndex % 2 == 1, tileTextureIndex == 2};
                    m_tiles[tileIndex].setTexture(m_texture);
                    m_tiles[tileIndex].setTextureRect(
                        sf::IntRect{
                            c_tileWidthi * (tileTextureIndex % c_tileMapWidth),
                            c_tileHeighti * (tileTextureIndex / c_tileMapWidth),
                            c_tileWidthi,
                            c_tileHeighti});
                    m_tiles[tileIndex].setPosition(
                        c_tileWidthf * (tileIndex % m_width),
                        c_tileHeightf * (tileIndex / m_width),
                        (tileIndex % m_width),
                        (tileIndex / m_width));

                    heights[x] = yEdge - 1;
                }
            }

            currentWidth = xEdge - 1;
        }
    }

    delete[] heights;
}

void Dungeon::populateWithEnemies()
{
    PRNG prng{};
    prng.seed64(1);
    prng.seed128(prng.nextSplitMix64(), prng.nextSplitMix64());

    m_numberOfEnemies = prng.random_roll(4, 18);

    int remainingEnemies = m_numberOfEnemies;
    for (int i = m_width + 5; i < m_numberOfTiles && remainingEnemies > 0; ++i)
    {
        if (prng.random_roll(50) == 1 && !m_tiles[i].hasCollision() && !m_tiles[i].hasUnit())
        {
            m_enemies.insert(std::pair(i, Enemy{m_tiles[i].getXCoord(), m_tiles[i].getYCoord(), *m_enemyTexture, *m_enemyTexture}));
            m_tiles[i].toggleUnit();
            --remainingEnemies;
        }
    }
}

bool Dungeon::los(sf::Vector2f currentTile, sf::Vector2f targetTile)
{
    if (currentTile.x == targetTile.x && currentTile.y == targetTile.y)
    { //tile is always in line-of-sight from itself
        return true;
    }

    float currentX = currentTile.x + 50.0f, currentY = currentTile.y + 50.0f;
    float targetX = targetTile.x + 50.0f, targetY = targetTile.y + 50.0f;
    float slope = (targetY - currentY) / (targetX - currentX);

    // walk the line from center of currentTile to the center of targetTile
    // checking whether each tile is invalid or collidable
    bool isLOS = true;
    int tileIndex;
    while (isLOS && currentX < targetX)
    {
        currentX += c_tileWidthf;
        currentY = targetY - (slope * (targetX - currentX));
        if (!isValidTile(sf::Vector2f{currentX, currentY}, tileIndex) || m_tiles[tileIndex].hasCollision())
        {
            isLOS = false;
        }
    }
    return isLOS;
}

void Dungeon::initialize(int width, int height, sf::Texture &texture, sf::Texture &enemyTexture)
{
    m_width = width;
    m_height = height;
    m_numberOfTiles = m_width * m_height;
    m_texture = &texture;
    m_enemyTexture = &enemyTexture;
    m_tiles = new Tile[m_numberOfTiles];

    generateProcedurally();
    populateWithEnemies();
}