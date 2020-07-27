// Copyright (c) 2018-2020 Marco Wang <m.aesophor@gmail.com>. All rights reserved.
#include "CommandParser.h"

#include <memory>

#include "character/Player.h"
#include "character/Npc.h"
#include "gameplay/DialogueTree.h"
#include "item/Item.h"
#include "map/GameMapManager.h"
#include "ui/dialogue/DialogueManager.h"
#include "ui/notifications/Notifications.h"
#include "util/StringUtil.h"
#include "util/Logger.h"

#define DEFAULT_ERR_MSG "unable to parse this line"

using std::string;
using std::vector;
using std::unique_ptr;
using std::out_of_range;
using std::invalid_argument;

using CmdTable = std::unordered_map<std::string,
      void (vigilante::CommandParser::*)(const std::vector<std::string>&)>;

namespace vigilante {

CommandParser::CommandParser() : _success(), _errMsg() {}

void CommandParser::parse(const string& cmd, bool showNotification) {
  vector<string> args;
  if (cmd.empty() || (args = string_util::split(cmd)).empty()) {
    return;
  }

  _success = false;
  _errMsg = DEFAULT_ERR_MSG;

  // Command handler table.
  static const CmdTable cmdTable = {
    {"startquest",         &CommandParser::startQuest        },
    {"additem",            &CommandParser::addItem           },
    {"removeitem",         &CommandParser::removeItem        },
    {"updateDialogueTree", &CommandParser::updateDialogueTree},
    {"followPlayer",       &CommandParser::followPlayer      },
  };
 
  // Execute the corresponding command handler from _cmdTable.
  // The obtained value from _cmdTable is a class member function pointer.
  CmdTable::const_iterator it = cmdTable.find(args[0]);
  if (it != cmdTable.end()) {
    (this->*((*it).second))(args);
  }

  if (!_success) {
    _errMsg = args[0] + ": " + _errMsg;
    VGLOG(LOG_ERR, "%s", _errMsg.c_str());
  }

  if (showNotification) {
    Notifications::getInstance()->show((_success) ? cmd : _errMsg);
  }
}

void CommandParser::setSuccess() {
  _success = true;
}

void CommandParser::setError(const string& errMsg) {
  _success = false;
  _errMsg = errMsg;
}


void CommandParser::startQuest(const vector<string>& args) {
  if (args.size() < 2) {
    setError("usage: startQuest <quest>");
    return;
  }

  GameMapManager::getInstance()->getPlayer()->getQuestBook().startQuest(args[1]);
  setSuccess();
}


void CommandParser::addItem(const vector<string>& args) {
  if (args.size() < 2) {
    setError("usage: addItem <itemName> [amount]");
    return;
  }

  int amount = 1;
  if (args.size() >= 3) {
    try {
      amount = std::stoi(args[2]);
    } catch (const invalid_argument& ex) {
      setError("invalid argument `amount`");
      return;
    } catch (const out_of_range& ex) {
      setError("`amount` is too large");
      return;
    } catch (...) {
      setError("unknown error");
      return;
    }
  }

  if (amount <= 0) {
    setError("`amount` has to be at least 1");
    return;
  }

  unique_ptr<Item> item = Item::create(args[1]);
  GameMapManager::getInstance()->getPlayer()->addItem(std::move(item), amount);
  setSuccess();
}


void CommandParser::removeItem(const vector<string>& args) {
  if (args.size() < 2) {
    setError("usage: removeItem <itemName> [amount]");
    return;
  }

  int amount = 1;
  if (args.size() >= 3) {
    try {
      amount = std::stoi(args[2]);
    } catch (const invalid_argument& ex) {
      setError("invalid argument `amount`");
      return;
    } catch (const out_of_range& ex) {
      setError("`amount` is too large");
      return;
    } catch (...) {
      setError("unknown error");
      return;
    }
  }

  if (amount <= 0) {
    setError("`amount` has to be at least 1");
    return;
  }

  unique_ptr<Item> item = Item::create(args[1]);
  GameMapManager::getInstance()->getPlayer()->removeItem(item.get(), amount);
  setSuccess();
}


void CommandParser::updateDialogueTree(const vector<string>& args) {
  if (args.size() < 3) {
    setError("usage: updateDialogueTree <npcJson> <dialogueTreeJson>");
    return;
  }

  // TODO: Maybe add some argument check here?
 
  DialogueTree::setLatestNpcDialogueTree(args[1], args[2]);
  setSuccess();
}


void CommandParser::followPlayer(const vector<string>&) {
  Player* player = GameMapManager::getInstance()->getPlayer();
  Npc* npc = DialogueManager::getInstance()->getTargetNpc();

  assert(player != nullptr && npc != nullptr);
  npc->setFollowee(player);
}

}  // namespace vigilante
