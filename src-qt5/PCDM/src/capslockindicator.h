// -*-c++-*-
/* Caps Lock Indicator:
*  Written by Trenton Schulz (nrc@norwegianrockcat.com) 2017
*  Copyright(c) 2017 by Trenton Schulz
*  Available under the 3-clause BSD license
*/
#ifndef CAPSLOCKINDICATOR_H
#define CAPSLOCKINDICATOR_H

#include <QLabel>

class QEvent;
// A Simple class that watchs the caps lock and shows it's state.

class CapsLockIndicator : public QLabel
{
  Q_OBJECT
  Q_PROPERTY(bool capsLockOn READ capsLockOn WRITE setCapsLockOn NOTIFY capsLockOnChanged)
public:
  explicit CapsLockIndicator(QWidget *parent = 0);
  void setCapsLockOn(bool capsDown);
  bool capsLockOn() const;

  // syncs the state of the caps lock by querying the OS,
  // normally platform specific.
  void syncCapsLockState();
public slots:
  void retranslateUi();

protected:
  void changeEvent(QEvent *ev);

signals:
  void capsLockOnChanged(bool capsOnState);

private:
  void updateView();
  bool mCapsLockOn;
};

#endif
