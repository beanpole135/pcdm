/* PCDM Login Manager:
*  Written by Ken Moore (ken@pcbsd.org) 2012/2013
*  Copyright(c) 2013 by the PC-BSD Project
*  Available under the 3-clause BSD license
*/

#ifndef PCDM_BACKEND_H
#define PCDM_BACKEND_H

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileSystemWatcher>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <QHash>

#include "pcdm-config.h"
//#include "pcbsd-utils.h"

#define PCSYSINSTALL    QString("/usr/sbin/pc-sysinstall")
#define DBDIR QString("/var/db/pcdm/")

class Process : public QProcess {
public:
    Process(const QStringList &args) {
        setReadChannel(QProcess::StandardOutput);
        start(PCSYSINSTALL, args);
    }
};

//Class for reading/listing the available users/info for the system
class UserList : public QObject{
	Q_OBJECT
public:
	UserList(QObject *parent = 0);
	~UserList();

	//Environment setting functions
	void allowUnder1K(bool allow); //flag to allow/show UID's under 1000 if other data checks out
	void allowRootUser(bool allow); //flag to permit login with the system "root" user (uid 0)
	void excludeUsers(QStringList exclude); //particular users which should always be excluded if their UID < 1000

	//Main start function (returns almost instantly)
	void updateList(); //results will be ready when signals are send out
	void stopUpdates(); //stop the periodic updates/checks

	//Get usernames
	QStringList users();
	QString findUser(QString displayname);
	//Return info for a username
	QString homedir(QString user);
	QString displayname(QString user);
	QString shell(QString user);
	QString status(QString user);
	bool isReady(QString user); //always true unless a personacrypt user
	
private:
	QProcess *syncProc, *userProc;
	QTimer *syncTimer, *userTimer;
	QHash<QString,QString> HASH;

	//Special flags for running user scans
	bool allowunder1kUID, allowroot;
	QStringList excludedUsers;

	bool parseUserLine(QString line, QStringList *oldusers, QStringList *allPC, QStringList *activePC); //returns true if data changed, and removes itself from oldusers as needed

private slots:
	void userProcFinished();
	void syncProcFinished();
	
	void startSyncProc(); //start probe for personacrypt user status
	void startUserProc(); //start probe for users

signals:
	void UsersChanged();
	void UserStatusChanged(QString, QString); //[user, status] emitted when a user status changes for personacrypt/syncthing
};

class Backend {
public:
    static QStringList getAvailableDesktops();
    static QString getDesktopComment(QString);
    static QString getNLDesktopName(QString);
    static QString getDesktopIcon(QString);
    static QString getDesktopBinary(QString);
    //static void allowUidUnder1K(bool allow, QStringList excludes = QStringList() );
    //static QStringList getSystemUsers(bool realnames = true);
    //static QString getUsernameFromDisplayname(QString);
    //static QString getDisplayNameFromUsername(QString);
    static QStringList keyModels();
    static QStringList keyLayouts();
    static QStringList keyVariants(const QString &layout, QStringList &savedKeyVariants);
    static bool changeKbMap(QString model, QString layout, QString variant);
    static QString resetKbdCmd(); //read the default settings, and return the command to run to re-set those settings
    static QStringList languages();
    static void openLogFile(QString);
    static void log(QString); 
    //static QString getUserHomeDir(QString);
    //static QString getUserShell(QString);
    static void checkLocalDirs();
    static QStringList runShellCommand(QString);

    //Auto-login usage
    static QString getALUsername();
    static QString getALDesktopCmd();
    static QString getALPassword();
    
    //Saved/Prior Settings
    static QString getLastUser();
    static QString getLastDE(QString, QString);
    static void saveLoginInfo(QString, QString, QString);
    static void loadDPIpreference();
    static void setDPIpreference(QString);
    static void readDefaultSysEnvironment(QString &lang, QString &keymodel, QString &keylayout, QString &keyvariant);
    static void saveDefaultSysEnvironment(QString lang, QString keymodel, QString keylayout, QString keyvariant);
    
    //Personacrypt usage
    static QStringList getRegisteredPersonaCryptUsers();
    static QStringList getAvailablePersonaCryptUsers();
    static bool MountPersonaCryptUser(QString user, QString pass);
    static bool UnmountPersonaCryptUser(QString user);
    
    static bool writeFile(QString fileName, QStringList contents);
    
private:	
    static void loadXSessionsData();
    static QStringList readXSessionsFile(QString, QString);
    //static void readSystemUsers(bool directfile = false);
    static void readSystemLastLogin();
    static void writeSystemLastLogin(QString, QString);
    static QString readUserLastDesktop(QString);
    static void writeUserLastDesktop(QString, QString);
};


#endif // PCDM_BACKEND_H
