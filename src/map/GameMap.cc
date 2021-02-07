// Copyright (c) 2018-2020 Marco Wang <m.aesophor@gmail.com>. All rights reserved.
#include "GameMap.h"

#include <thread>

#include "std/make_unique.h"
#include "AssetManager.h"
#include "CallbackManager.h"
#include "Constants.h"
#include "character/Character.h"
#include "character/Player.h"
#include "character/Npc.h"
#include "character/Party.h"
#include "item/Equipment.h"
#include "item/Consumable.h"
#include "map/GameMapManager.h"
#include "map/object/Chest.h"
#include "ui/Shade.h"
#include "ui/notifications/Notifications.h"
#include "util/box2d/b2BodyBuilder.h"
#include "util/JsonUtil.h"
#include "util/RandUtil.h"

using std::vector;
using std::unordered_set;
using std::string;
using std::thread;
using std::unique_ptr;
using std::shared_ptr;
using cocos2d::Director;
using cocos2d::TMXTiledMap;
using cocos2d::TMXObjectGroup;
using cocos2d::Sequence;
using cocos2d::FadeIn;
using cocos2d::FadeOut;
using cocos2d::CallFunc;

namespace vigilante {

GameMap::GameMap(b2World* world, const string& tmxMapFileName)
    : _world(world),
      _tmxTiledMap(TMXTiledMap::create(tmxMapFileName)),
      _tmxTiledMapFileName(tmxMapFileName),
      _dynamicActors(),
      _portals() {}


void GameMap::createObjects() {
  // Create box2d objects from layers.
  createPolylines("Ground", category_bits::kGround, true, kGroundFriction);
  createPolylines("Wall", category_bits::kWall, true, kWallFriction);
  createRectangles("Platform", category_bits::kPlatform, true, kGroundFriction);
  createPolylines("PivotMarker", category_bits::kPivotMarker, false, 0);
  createPolylines("CliffMarker", category_bits::kCliffMarker, false, 0);
  
  spawnPortals();
  spawnChests();
  spawnNpcs();
}

void GameMap::deleteObjects() {
  // Destroy ground, walls, platforms and portal bodies.
  for (auto body : _tmxTiledMapBodies) {
    _world->DestroyBody(body);
  }

  // Destroy DynamicActor's b2body and textures.
  for (auto& actor : _dynamicActors) {
    actor->removeFromMap();
  }
}

unique_ptr<Player> GameMap::spawnPlayer() const {
  auto player = std::make_unique<Player>(asset_manager::kPlayerJson);

  const TMXObjectGroup* objGroup = _tmxTiledMap->getObjectGroup("Player");
  const auto& valMap = objGroup->getObjects().at(0).asValueMap();
  float x = valMap.at("x").asFloat();
  float y = valMap.at("y").asFloat();

  player->showOnMap(x, y);
  return player;
}

Item* GameMap::spawnItem(const string& itemJson, float x, float y, int amount) {
  Item* item = showDynamicActor<Item>(Item::create(itemJson), x, y);
  item->setAmount(amount);

  float offsetX = rand_util::randFloat(-.3f, .3f);
  float offsetY = 3.0f;
  item->getBody()->ApplyLinearImpulse({offsetX, offsetY}, item->getBody()->GetWorldCenter(), true);

  return item;
}


unordered_set<b2Body*>& GameMap::getTmxTiledMapBodies() {
  return _tmxTiledMapBodies;
}

const string& GameMap::getTmxTiledMapFileName() const {
  return _tmxTiledMapFileName;
}

TMXTiledMap* GameMap::getTmxTiledMap() const {
  return _tmxTiledMap;
}

float GameMap::getWidth() const {
  return _tmxTiledMap->getMapSize().width * _tmxTiledMap->getTileSize().width;
}

float GameMap::getHeight() const {
  return _tmxTiledMap->getMapSize().height * _tmxTiledMap->getTileSize().height;
}


void GameMap::createRectangles(const string& layerName, short categoryBits,
                               bool collidable, float friction) {
  TMXObjectGroup* objGroup = _tmxTiledMap->getObjectGroup(layerName);
  //log("%s\n", _map->getProperty("backgroundMusic").asString().c_str());
  
  for (const auto& rectObj : objGroup->getObjects()) {
    const auto& valMap = rectObj.asValueMap();
    float x = valMap.at("x").asFloat();
    float y = valMap.at("y").asFloat();
    float w = valMap.at("width").asFloat();
    float h = valMap.at("height").asFloat();

    b2BodyBuilder bodyBuilder(_world);

    b2Body* body = bodyBuilder.type(b2BodyType::b2_staticBody)
      .position(x + w / 2, y + h / 2, kPpm)
      .buildBody();

    bodyBuilder.newRectangleFixture(w / 2, h / 2, kPpm)
      .categoryBits(categoryBits)
      .setSensor(!collidable)
      .friction(friction)
      .buildFixture();

    _tmxTiledMapBodies.insert(body);
  }
}

void GameMap::createPolylines(const string& layerName, short categoryBits,
                              bool collidable, float friction) {
  float scaleFactor = Director::getInstance()->getContentScaleFactor();

  for (const auto& lineObj : _tmxTiledMap->getObjectGroup(layerName)->getObjects()) {
    const auto& valMap = lineObj.asValueMap();
    float xRef = valMap.at("x").asFloat();
    float yRef = valMap.at("y").asFloat();

    const auto& valVec = valMap.at("polylinePoints").asValueVector();
    b2Vec2 vertices[valVec.size()];
    for (size_t i = 0; i < valVec.size(); i++) {
      float x = valVec.at(i).asValueMap().at("x").asFloat() / scaleFactor;
      float y = valVec.at(i).asValueMap().at("y").asFloat() / scaleFactor;
      vertices[i] = {xRef + x, yRef - y};
    }

    b2BodyBuilder bodyBuilder(_world);

    b2Body* body = bodyBuilder.type(b2BodyType::b2_staticBody)
      .position(0, 0, kPpm)
      .buildBody();

    bodyBuilder.newPolylineFixture(vertices, valVec.size(), kPpm)
      .categoryBits(categoryBits)
      .setSensor(!collidable)
      .friction(friction)
      .buildFixture();

    _tmxTiledMapBodies.insert(body);
  }
}

void GameMap::spawnPortals() {
  for (const auto& rectObj : _tmxTiledMap->getObjectGroup("Portal")->getObjects()) {
    const auto& valMap = rectObj.asValueMap();
    float x = valMap.at("x").asFloat();
    float y = valMap.at("y").asFloat();
    float w = valMap.at("width").asFloat();
    float h = valMap.at("height").asFloat();
    string targetTmxMapFilePath = valMap.at("targetMap").asString();
    int targetPortalId = valMap.at("targetPortalID").asInt();
    bool willInteractOnContact = valMap.at("willInteractOnContact").asBool();

    b2BodyBuilder bodyBuilder(_world);

    b2Body* body = bodyBuilder.type(b2BodyType::b2_staticBody)
      .position(x + w / 2, y + h / 2, kPpm)
      .buildBody();

    _portals.push_back(std::make_unique<Portal>(targetTmxMapFilePath, targetPortalId,
                                                willInteractOnContact, true, body));

    bodyBuilder.newRectangleFixture(w / 2, h / 2, kPpm)
      .categoryBits(category_bits::kPortal)
      .setSensor(true)
      .friction(0)
      .setUserData(_portals.back().get())
      .buildFixture();
  }
}

void GameMap::spawnNpcs() {
  auto player = GameMapManager::getInstance()->getPlayer();

  for (const auto& rectObj : _tmxTiledMap->getObjectGroup("Npcs")->getObjects()) {
    const auto& valMap = rectObj.asValueMap();
    float x = valMap.at("x").asFloat();
    float y = valMap.at("y").asFloat();
    string json = valMap.at("json").asString();

    // Skip this character (don't spawn it)
    // if it has already been recruited by the player.
    // FIXME: what if two or more characters with the same name exist in one map?
    if (player && (player->getParty()->hasDeceasedMember(json) ||
                   player->getParty()->hasMember(json))) {
      continue;
    }
    showDynamicActor(std::make_shared<Npc>(json), x, y);
  }


  if (player) {
    for (const auto& p : player->getParty()->getWaitingMembersLocationInfo()) {
      const string& characterJsonFileName = p.first;
      const Party::WaitingLocationInfo& location = p.second;

      // If this Npc is waiting for its leader in the current map,
      // then we should show it on this map.
      if (location.tmxMapFileName == _tmxTiledMapFileName) {
        player->getParty()->getMember(characterJsonFileName)->showOnMap(location.x * kPpm,
                                                                        location.y * kPpm);
      }
    }
  }
}

void GameMap::spawnChests() {
  for (const auto& rectObj : _tmxTiledMap->getObjectGroup("Chest")->getObjects()) {
    const auto& valMap = rectObj.asValueMap();
    float x = valMap.at("x").asFloat();
    float y = valMap.at("y").asFloat();
    string items = valMap.at("items").asString();
    showDynamicActor(std::make_shared<Chest>(items), x, y);
  }
}


GameMap::Portal::Portal(const string& targetTmxMapFileName, int targetPortalId,
                        bool willInteractOnContact, bool isLocked, b2Body* body)
    : _targetTmxMapFileName(targetTmxMapFileName),
      _targetPortalId(targetPortalId),
      _willInteractOnContact(willInteractOnContact),
      _isLocked(isLocked),
      _body(body),
      _hintBubbleFxSprite() {}

GameMap::Portal::~Portal() {
  _body->GetWorld()->DestroyBody(_body);
}

void GameMap::Portal::onInteract(Character* user) {
  if (isLocked()) {
    if (!canBeUnlockedBy(user)) {
      Notifications::getInstance()->show("This door is locked.");
      return;
    } else {
      Notifications::getInstance()->show("Door unlocked.");
      unlock();
    }
  }

  // [IMPORTANT]
  // Before loading the new GameMap, we need to make sure that
  // all pending callbacks have finished executing.

  // Pause all Npcs from acting, which prevents new callbacks
  // from being generated.
  Npc::setNpcsAllowedToAct(false);

  Shade::getInstance()->getImageView()->runAction(Sequence::createWithTwoActions(
      FadeIn::create(Shade::_kFadeInTime),
      CallFunc::create([this, user]() {
        // We'll create another thread and use spinlock to wait until
        // all callbacks have finished executing before loading the new GameMap
        // and faded out the shade.
        thread([this, user]() {
          while (CallbackManager::getInstance()->getPendingCount() > 0);

          Shade::getInstance()->getImageView()->runAction(Sequence::create(
              CallFunc::create([this, user]() {
                // Load target GameMap.
                // Note that after calling GameMapManager::loadGameMap(),
                // all instances of Portal in current GameMap will be freed,
                // so we MUST NOT refer to this->blah anymore, or there'll be UAF bugs.
                const string newMapFileName = _targetTmxMapFileName;
                const int targetPortalId = _targetPortalId;
                const GameMap* newMap = GameMapManager::getInstance()->loadGameMap(newMapFileName);
                const b2Vec2 portalPos = newMap->_portals.at(targetPortalId)->_body->GetPosition();

                // Place the user and its party members at the portal.
                user->setPosition(portalPos.x, portalPos.y);

                // How should we handle user's allies?
                // (1) If `ally` is not waiting for its party leader,
                //     then teleport the ally's body to its party leader.
                // (2) If `ally` is waiting for its party leader,
                //     AND if this new map is not where `ally` is waiting at,
                //     then we'll remove it from the map temporarily.
                //     Whether it will be shown again is determined in
                //     GameMap::spawnNpcs().
                for (auto ally : user->getAllies()) {
                  assert(ally->getParty() != nullptr);

                  if (!ally->isWaitingForPartyLeader()) {
                    ally->setPosition(portalPos.x, portalPos.y);
                  } else if (newMapFileName != ally->getParty()->getWaitingMemberLocationInfo(
                        ally->getCharacterProfile().jsonFileName).tmxMapFileName) {
                    ally->removeFromMap();
                  }
                }
              }),
              FadeOut::create(Shade::_kFadeOutTime),
              nullptr
            )
          );

          // Resume Npcs to act.
          Npc::setNpcsAllowedToAct(true);
        }).detach();
      })
    )
  );
}

void GameMap::Portal::createHintBubbleFx() {
  if (_hintBubbleFxSprite) {
    removeHintBubbleFx();
  }

  const b2Vec2& bodyPos = _body->GetPosition();
  float x = bodyPos.x * kPpm;
  float y = bodyPos.y * kPpm + HINT_BUBBLE_FX_SPRITE_OFFSET_Y;

  _hintBubbleFxSprite
    = GameMapManager::getInstance()->getFxManager()->createFx(
        "Texture/fx/hint_bubble", "portal_available", x, y, -1, 45.0f);
}

void GameMap::Portal::removeHintBubbleFx() {
  if (!_hintBubbleFxSprite) {
    return;
  }

  _hintBubbleFxSprite->stopAllActions();
  _hintBubbleFxSprite->removeFromParent();
  _hintBubbleFxSprite = nullptr;
}


bool GameMap::Portal::canBeUnlockedBy(Character* user) const {
  return true;
}

bool GameMap::Portal::isLocked() const {
  return _isLocked;
}

void GameMap::Portal::lock() {
  _isLocked = true;
}

void GameMap::Portal::unlock() {
  _isLocked = false;
}


const string& GameMap::Portal::getTargetTmxMapFileName() const {
  return _targetTmxMapFileName;
}

int GameMap::Portal::getTargetPortalId() const {
  return _targetPortalId;
}

bool GameMap::Portal::willInteractOnContact() const {
  return _willInteractOnContact;
}

}  // namespace vigilante
