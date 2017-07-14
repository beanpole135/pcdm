/* Caps Lock Indicator:
*  Written by Trenton Schulz (nrc@norwegianrockcat.com) 2017
*  Copyright(c) 2017 by Trenton Schulz
*  Available under the 3-clause BSD license
*/
#include "capslockindicator.h"
#include <QX11Info>
#include <QEvent>

#include <X11/XKBlib.h>

CapsLockIndicator::CapsLockIndicator(QWidget *parent)
  : QLabel(parent)
{
  setAlignment(Qt::AlignCenter);
  // prime the pump by querying the operating system
  syncCapsLockState();
}

void CapsLockIndicator::updateView()
{
  if (mCapsLockOn) {
    setText(tr("Caps Lock is ON!"));
  } else {
    clear();
  }
  setVisible(mCapsLockOn);
}

void CapsLockIndicator::setCapsLockOn(bool capsDown)
{
  if (capsDown != mCapsLockOn) {
    mCapsLockOn = capsDown;
    updateView();
    emit capsLockOnChanged(mCapsLockOn);
  }
}

bool CapsLockIndicator::capsLockOn() const
{
  return mCapsLockOn;
}

void CapsLockIndicator::changeEvent(QEvent *event)
{
  if (event->type() == QEvent::ActivationChange) {
    syncCapsLockState();
  }
  QLabel::changeEvent(event);
}

static int CapsLockBit = 0x01;

void CapsLockIndicator::syncCapsLockState()
{

  // This code is X11 specific at the moment, but since this is
  // targetting X11 at the moment, we won't protect it with Q_WS_X11
  // stuff for now.
  // This code is based on info from the Qt Forums
  unsigned int state;
  XkbGetIndicatorState(QX11Info::display(), XkbUseCoreKbd, &state);
  mCapsLockOn = (state & CapsLockBit) == 1;
  updateView();
}

void CapsLockIndicator::retranslateUi()
{
  updateView();
}
