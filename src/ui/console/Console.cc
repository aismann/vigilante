// Copyright (c) 2018-2020 Marco Wang <m.aesophor@gmail.com>. All rights reserved.
#include "Console.h"

#include "input/InputManager.h"
#include "util/Logger.h"

#define CONSOLE_X 10
#define CONSOLE_Y 10
#define DEFAULT_HISTORY_SIZE 32

using std::string;
using cocos2d::Layer;
using cocos2d::Event;
using cocos2d::EventKeyboard;

namespace vigilante {

Console* Console::getInstance() {
  static Console instance;
  return &instance;
}

Console::Console()
    : _layer(Layer::create()),
      _textField(),
      _cmdParser(),
      _cmdHistory() {

  auto onSubmit = [this]() {
    executeCmd(_textField.getString(), /*showNotification=*/true);
    _textField.clear();
  };

  auto onDismiss = [this]() {
    setVisible(false);
  };

  auto extraOnKeyPressed = [this](EventKeyboard::KeyCode keyCode, Event*) {
    if (keyCode == EventKeyboard::KeyCode::KEY_UP_ARROW && _cmdHistory.canGoBack()) {
      _cmdHistory.goBack();
      _textField.setString(_cmdHistory.getCurrentLine());
    } else if (keyCode == EventKeyboard::KeyCode::KEY_DOWN_ARROW && _cmdHistory.canGoForward()) {
      _cmdHistory.goForward();
      _textField.setString(_cmdHistory.getCurrentLine());
    }
  };

  _textField.setOnSubmit(onSubmit);
  _textField.setOnDismiss(onDismiss);
  _textField.setExtraOnKeyPressed(extraOnKeyPressed);
  _textField.setDismissKey(EventKeyboard::KeyCode::KEY_GRAVE);

  _layer->setVisible(false);
  _layer->setPosition(CONSOLE_X, CONSOLE_Y);
  _layer->addChild(_textField.getLayout());
}


void Console::update(float delta) {
  if (!_layer->isVisible()) {
    return;
  }
  _textField.update(delta);
}


void Console::executeCmd(const string& cmd, bool showNotification) {
  VGLOG(LOG_INFO, "Executing: %s", cmd.c_str());
  _cmdParser.parse(cmd, showNotification);
  _cmdHistory.push(cmd);
  _cmdHistory._current = _cmdHistory._tail;
}


bool Console::isVisible() const {
  return _layer->isVisible();
}

void Console::setVisible(bool visible) {
  _layer->setVisible(visible);
  _textField.setReceivingInput(visible);
}

Layer* Console::getLayer() const {
  return _layer;
}


Console::CommandHistory::CommandHistory()
    : CircularBuffer<string>(DEFAULT_HISTORY_SIZE), _current(_tail) {}

bool Console::CommandHistory::canGoBack() const {
  return _current != _head;
}

bool Console::CommandHistory::canGoForward() const {
  return _current != _tail;
}

void Console::CommandHistory::goBack() {
  _current--;
  if (_current < 0) {
    _current = capacity() - 1;
  }
}

void Console::CommandHistory::goForward() {
  _current++;
  if (_current >= (int) capacity()) {
    _current = 0;
  }
}

const string& Console::CommandHistory::getCurrentLine() const {
  return _data[_current];
}

} // namespace vigilante
