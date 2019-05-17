#include "Enemy.h"

#include "GameAssetManager.h"
#include "map/GameMapManager.h"
#include "util/box2d/b2BodyBuilder.h"
#include "util/CallbackUtil.h"
#include "util/CategoryBits.h"
#include "util/Constants.h"

using std::string;
using cocos2d::Vector;
using cocos2d::Director;
using cocos2d::Repeat;
using cocos2d::RepeatForever;
using cocos2d::Animation;
using cocos2d::Animate;
using cocos2d::Action;
using cocos2d::Sprite;
using cocos2d::SpriteFrame;
using cocos2d::SpriteFrameCache;
using cocos2d::SpriteBatchNode;
using vigilante::category_bits::kPlayer;
using vigilante::category_bits::kPortal;
using vigilante::category_bits::kEnemy;
using vigilante::category_bits::kMeleeWeapon;
using vigilante::category_bits::kItem;
using vigilante::category_bits::kGround;
using vigilante::category_bits::kPlatform;
using vigilante::category_bits::kCliffMarker;
using vigilante::category_bits::kWall;
using vigilante::category_bits::kEnemy;
using vigilante::category_bits::kObject;


namespace vigilante {

Enemy::Enemy(const std::string& name, float x, float y) : Character(name, x, y), Bot(this) {
  short bodyCategoryBits = kEnemy;
  short bodyMaskBits = kPortal | kPlayer | kMeleeWeapon | kCliffMarker;
  short feetMaskBits = kGround | kPlatform | kWall | kItem;
  short weaponMaskBits = kPlayer | kObject;
  defineBody(b2BodyType::b2_dynamicBody, bodyCategoryBits, bodyMaskBits, feetMaskBits, weaponMaskBits, x, y);
  defineTexture(asset_manager::kPlayerSpritesheet, x, y);
}


void Enemy::update(float delta) {
  Character::update(delta);
  act(delta);
}

void Enemy::receiveDamage(Character* source, int damage) {
  Character::receiveDamage(source, damage);
  _isAlerted = true;
}

} // namespace vigilante
