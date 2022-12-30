// Copyright (c) 2018-2021 Marco Wang <m.aesophor@gmail.com>. All rights reserved.
#include "CollectItemObjective.h"

#include "character/Player.h"
#include "item/Item.h"
#include "map/GameMapManager.h"

using namespace std;

namespace vigilante {

CollectItemObjective::CollectItemObjective(const string& desc,
                                           const string& itemName,
                                           int amount)
    : Quest::Objective(Quest::Objective::Type::COLLECT, desc),
      _itemName(itemName),
      _amount(amount) {}

bool CollectItemObjective::isCompleted() const {
  return GameMapManager::getInstance()->getPlayer()->getItemAmount(_itemName) >= _amount;
}

}  // namespace vigilante
