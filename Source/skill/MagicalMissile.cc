// Copyright (c) 2018-2021 Marco Wang <m.aesophor@gmail.com>. All rights reserved.
#include "MagicalMissile.h"

#include <functional>
#include <memory>

#include "Audio.h"
#include "CallbackManager.h"
#include "Constants.h"
#include "character/Character.h"
#include "scene/GameScene.h"
#include "scene/SceneManager.h"
#include "util/B2BodyBuilder.h"
#include "util/Logger.h"

using namespace std;
using namespace vigilante::category_bits;
USING_NS_AX;

namespace vigilante {

namespace {

constexpr int kMagicalMissleNumAnimations = MagicalMissile::AnimationType::SIZE;
constexpr int kMagicalMissleNumFixtures = 1;

constexpr auto kMagicalMissleCategoryBits = kProjectile;
constexpr auto kMagicalMissleMaskBits = kPlayer | kEnemy | kWall;

}  // namespace

MagicalMissile::MagicalMissile(const string& jsonFileName, Character* user, const bool onGround)
    : DynamicActor{kMagicalMissleNumAnimations, kMagicalMissleNumFixtures},
      _skillProfile{jsonFileName},
      _user{user},
      _isOnGround{onGround} {}

bool MagicalMissile::showOnMap(float x, float y) {
  if (_isShownOnMap) {
    return false;
  }

  _isShownOnMap = true;

  defineBody(b2BodyType::b2_kinematicBody, x, y,
             kMagicalMissleCategoryBits,
             kMagicalMissleMaskBits);

  defineTexture(_skillProfile.textureResDir, x, y);

  _node->addChild(_bodySpritesheet, graphical_layers::kSpell);

  auto gmMgr = SceneManager::the().getCurrentScene<GameScene>()->getGameMapManager();
  gmMgr->getLayer()->addChild(_node, graphical_layers::kSpell);

  return true;
}

void MagicalMissile::update(const float delta) {
  if (_hasHit) {
    return;
  }

  const b2Vec2& b2bodyPos = _body->GetPosition();
  _bodySprite->setPosition(b2bodyPos.x * kPpm + _skillProfile.spriteOffsetX,
                           b2bodyPos.y * kPpm + _skillProfile.spriteOffsetY);

  auto gmMgr = SceneManager::the().getCurrentScene<GameScene>()->getGameMapManager();
  GameMap* gameMap = gmMgr->getGameMap();

  const float x = _body->GetPosition().x * kPpm;
  const float y = _body->GetPosition().y * kPpm;
  if (x < 0 || y < 0 || x > gameMap->getWidth() || y > gameMap->getHeight()) {
    onHit(nullptr);
  }
}

int MagicalMissile::getDamage() const {
  return _skillProfile.physicalDamage + _skillProfile.magicalDamage;
}

void MagicalMissile::onHit(Character* target) {
  if (_hasHit) {
    return;
  }

  _hasHit = true;
  _body->SetLinearVelocity({_body->GetLinearVelocity().x / 2, 0});

  auto gmMgr = SceneManager::the().getCurrentScene<GameScene>()->getGameMapManager();
  _bodySprite->runAction(Sequence::createWithTwoActions(
    Animate::create(_bodyAnimations[AnimationType::ON_HIT]),
    CallFunc::create([=]() {
      gmMgr->getGameMap()->removeDynamicActor(this);
      _user->removeActiveSkill(this);
    })
  ));

  if (target) {
    const bool isFacingRight = _body->GetLinearVelocity().x > 0;
    const float knockBackForceX = isFacingRight ? 3.5f : -3.5f;
    const float knockBackForceY = 1.0f;
    _user->knockBack(target, knockBackForceX, knockBackForceY);
    _user->inflictDamage(target, getDamage());

    target->setStunned(true);
    CallbackManager::the().runAfter([target](const CallbackManager::CallbackId) {
      target->setStunned(false);
    }, 2.0f);
  }

  // Play sound effect.
  Audio::the().playSfx(_skillProfile.sfxHit);
}

void MagicalMissile::import(const string& jsonFileName) {
  _skillProfile = Skill::Profile(jsonFileName);
}

bool MagicalMissile::canActivate() {
  return _user->getCharacterProfile().magicka + _skillProfile.deltaMagicka >= 0;
}

void MagicalMissile::activate() {
  // Make sure this instance is only activated once.
  if (_hasActivated) {
    return;
  }

  _hasActivated = true;

  // Modify character's stats.
  _user->getCharacterProfile().magicka += _skillProfile.deltaMagicka;

  CallbackManager::the().runAfter([this](const CallbackManager::CallbackId) {
    shared_ptr<Skill> activeCopy = _user->getActiveSkill(this);
    auto actor = std::dynamic_pointer_cast<DynamicActor>(activeCopy);
    if (!actor) {
      VGLOG(LOG_ERR, "Failed to copy skill: [%s].", _skillProfile.name.c_str());
      return;
    }

    auto gmMgr = SceneManager::the().getCurrentScene<GameScene>()->getGameMapManager();
    const float x = _user->getBody()->GetPosition().x;
    const float y = _user->getBody()->GetPosition().y;
    const float atkRange = _user->getCharacterProfile().attackRange;
    const float userBodyHeight = _user->getCharacterProfile().bodyHeight;
    const float offsetX = _user->isFacingRight() ? atkRange : -atkRange;
    const float offsetY = _isOnGround ? -userBodyHeight / 2: 0;
    gmMgr->getGameMap()->showDynamicActor(std::move(actor), x, y + offsetY / kPpm);

    // Set up kinematicBody's moving speed.
    _flyingSpeed = (_user->isFacingRight()) ? 4 : -4;
    _body->SetLinearVelocity({_flyingSpeed, 0});

    if (!_user->isFacingRight()) {
      _launchFxSprite->setFlippedX(true);
      _bodySprite->setFlippedX(true);
    }

    // Play the magical missile body's animation.
    _bodySprite->runAction(Animate::create(_bodyAnimations[AnimationType::FLYING]));

    // Play launch fx animation.
    _launchFxSprite->setPosition((x + offsetX) * kPpm, (y + offsetY) * kPpm);
    _launchFxSprite->runAction(Sequence::createWithTwoActions(
      Animate::create(_bodyAnimations[AnimationType::LAUNCH_FX]),
      CallFunc::create([=]() {
        _bodySpritesheet->removeChild(_launchFxSprite, true);
      })
    ));
  }, 0.8f);
}

string MagicalMissile::getIconPath() const {
  return _skillProfile.textureResDir + "/icon.png";
}

void MagicalMissile::defineBody(b2BodyType bodyType,
                                float x,
                                float y,
                                short categoryBits,
                                short maskBits) {
  float spellOffset = _user->getCharacterProfile().attackRange / kPpm;
  spellOffset = (_user->isFacingRight()) ? spellOffset : -spellOffset;

  b2World* world = _user->getBody()->GetWorld();
  B2BodyBuilder bodyBuilder(world);
  _body = bodyBuilder.type(bodyType)
    .position(x + spellOffset, y, 1)
    .buildBody();

  float scaleFactor = Director::getInstance()->getContentScaleFactor();
  b2Vec2 vertices[] = {
    {-10.0f / scaleFactor,  0.0f / scaleFactor},
    {  0.0f / scaleFactor, -2.0f / scaleFactor},
    { 10.0f / scaleFactor,  0.0f / scaleFactor},
    {  0.0f / scaleFactor,  2.0f / scaleFactor}
  };

  _fixtures[0] = bodyBuilder.newPolygonFixture(vertices, 4, kPpm)
    .categoryBits(categoryBits)
    .maskBits(maskBits)
    .setUserData(this)
    .buildFixture();
}

void MagicalMissile::defineTexture(const string& textureResDir, float x, float y) {
  _bodySpritesheet = SpriteBatchNode::create(textureResDir + "/spritesheet.png");

  _bodyAnimations[AnimationType::LAUNCH_FX] = createAnimation(textureResDir, "launch", 5.0f / kPpm);
  _bodyAnimations[AnimationType::FLYING] = createAnimation(textureResDir, "flying", 1.0f / kPpm);
  _bodyAnimations[AnimationType::ON_HIT] = createAnimation(textureResDir, "on_hit", 8.0f / kPpm);

  // Select a frame as default look for this sprite.
  string frameNamePrefix = StaticActor::getLastDirName(textureResDir);
  _launchFxSprite = Sprite::createWithSpriteFrameName(frameNamePrefix + "_launch/0.png");
  _launchFxSprite->setPosition(x, y);
  _bodySprite = Sprite::createWithSpriteFrameName(frameNamePrefix + "_flying/0.png");
  _bodySprite->setPosition(x, y);
  _bodySprite->setScaleX(_skillProfile.spriteScaleX);
  _bodySprite->setScaleY(_skillProfile.spriteScaleY);

  _bodySpritesheet->addChild(_launchFxSprite);
  _bodySpritesheet->addChild(_bodySprite);
  _bodySpritesheet->getTexture()->setAliasTexParameters();
}

}  // namespace vigilante
