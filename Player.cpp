#include "Player.h"

Player::Player(int x, int y, sf::Texture &activeTexture, sf::Texture &inactiveTexture, std::vector<Attack> &attacks)
    : Unit{x, y, activeTexture, inactiveTexture, true}, m_maxAttackPoints{1}, m_currentAttackPoints{1}, m_minRange{1}, m_maxRange{1}
{
    m_attacks = &attacks;
    for (auto &&attack : *m_attacks)
    {
        if (attack.getMinRange() < m_minRange)
        {
            m_minRange = attack.getMinRange();
        }
        if (attack.getMaxRange() > m_maxRange)
        {
            m_maxRange = attack.getMaxRange();
        }
    }
}

bool Player::chooseAttack(sf::Vector2f clickPosition)
{
    for (auto &&attack : *m_attacks)
    {
        if (attack.getGlobalBounds().contains(clickPosition))
        {
            m_isAttacking = true;
            m_selectedAttack = &attack;
            return true;
        }
    }
    return false;
}

void Player::stopAttack()
{
    m_isAttacking = false;
}

AttackResult Player::finishAttack(int attackRoll)
{
    AttackResult result;
    m_currentAttackPoints -= m_selectedAttack->getCost();
    if (attackRoll > m_target->getDefense())
    {
        int xpReceived = m_target->takeDamage(m_selectedAttack->getDamage());
        m_experiencePoints += xpReceived;
        if (m_target->isAlive())
        {
            result = AttackResult::Hit;
        }
        else
        {
            result = AttackResult::Kill;
        }
        if (m_experiencePoints >= c_xpToLevel[m_level - 1])
        {
            // todo: gain level if enough XP gained
        }
    }
    else
    {
        result = AttackResult::Miss;
    }
    stopAttack();
    clearTarget();

    return result;
}

int Player::getDistanceFromTarget(Unit &target)
{
    return abs(getXCoord() - target.getXCoord()) + abs(getYCoord() - target.getYCoord());
}

void Player::setTarget(Unit *target)
{
    m_target = target;
    m_hasTarget = true;
}

void Player::clearTarget()
{
    m_target = nullptr;
    m_hasTarget = false;
}

void Player::startTurn()
{
    Unit::startTurn();
    m_currentAttackPoints = m_maxAttackPoints;
    clearTarget();
}

void Player::endTurn()
{
    Unit::endTurn();
    clearTarget();
    stopAttack();
}

void Player::draw(sf::RenderWindow &window)
{
    window.draw(*this);
    if (m_hasTarget && !m_isAttacking)
    {
        int targetDistance = getDistanceFromTarget(*m_target);
        int j = 0;
        for (auto &&attack : *m_attacks)
        {
            if (attack.getMinRange() <= targetDistance && attack.getMaxRange() >= targetDistance)
            {
                attack.setPosition(getPosition().x - (c_attackTileWidth * (j++ + 1)), getPosition().y - c_attackTileWidth);
                window.draw(attack);
            }
        }
    }
}