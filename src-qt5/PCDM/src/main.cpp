/* PCDM Login Manager:
*  Written by Ken Moore (ken@pcbsd.org) 2012/2013
*  Copyright(c) 2013 by the PC-BSD Project
*  Available under the 3-clause BSD license
*/

#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QDesktopWidget>
#include <QFile>
#include <QSplashScreen>
#include <QTime>
#include <QDebug>
#include <QX11Info>
#include <QTextCodec>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

//#define TMPLANGFILE QString("/var/tmp/.PCDMLang")
#define TMPAUTOLOGINFILE QString("/var/tmp/.PCDMAutoLogin-$1")
#define TMPAUTHFILE QString("/var/tmp/.PCDMAuth-%1")
#define TMPSTOPFILE QString("/var/tmp/.PCDMstop-%1")

#include "pcdm-gui.h"
#include "pcdm-backend.h"
#include "pcdm-config.h"
#include "pcdm-xprocess.h"
#include "pcdm-logindelay.h"

//bool USECLIBS=false;


//Global classes
UserList *USERS = 0;
QString VT;

//Setup any qDebug/qWarning/qError messages to get saved into this log file directly
QFile logfile;
void MessageOutput(QtMsgType type, const QMessageLogContext &, const QString &msg){
  QString txt;
  switch(type){
  case QtDebugMsg:
  	  txt = QString("Debug: %1").arg(msg);
  	  break;
  case QtWarningMsg:
  	  txt = QString("Warning: %1").arg(msg);
  	  break;
  case QtCriticalMsg:
  	  txt = QString("CRITICAL: %1").arg(msg);
  	  break;
  case QtFatalMsg:
  	  txt = QString("FATAL: %1").arg(msg);
  	  break;
  default:
    txt = msg;
  }

  QTextStream out(&logfile);
  out << txt;
  if(!txt.endsWith("\n")){ out << "\n"; }
}
int runSingleSession(int argc, char *argv[]){
  //QTime clock;
  //clock.start();
  Backend::checkLocalDirs();  // Create and fill "/usr/local/share/PCDM" if needed
  //Setup the log file
  logfile.setFileName("/var/log/PCDM-"+VT+".log");
  qDebug() << "PCDM Log File: " << logfile.fileName(); //This does not go into the log
    if(QFile::exists(logfile.fileName()+".old")){ QFile::remove(logfile.fileName()+".old"); }
    if(logfile.exists()){ QFile::rename(logfile.fileName(), logfile.fileName()+".old"); }
      //Make sure the parent directory exists
      if(!QFile::exists("/var/log")){
        QDir dir;
        dir.mkpath("/var/log");
      }
    logfile.open(QIODevice::WriteOnly | QIODevice::Append);
  //Backend::openLogFile("/var/log/PCDM.log");

  //qDebug() << "Backend Checks Finished:" << QString::number(clock.elapsed())+" ms";
  Backend::loadDPIpreference();
  //Setup the initial system environment (locale, keyboard)
  QString lang, kmodel, klayout, kvariant;
  Backend::readDefaultSysEnvironment(lang,kmodel,klayout,kvariant);
  lang = lang.section(".",0,0).simplified(); //just in case it also had the encoding saved to the file
  if(lang.isEmpty() || lang.toLower()=="c"){ lang = "en_US"; }
  setenv("LANG", lang.toUtf8(), 0);
  Backend::changeKbMap(kmodel,klayout,kvariant);
  //Check for the flag to try and auto-login
  bool ALtriggered = false;
  if(QFile::exists(TMPAUTOLOGINFILE.arg(VT))){ ALtriggered=true; QFile::remove(TMPAUTOLOGINFILE.arg(VT)); }

  //QString changeLang;
  // Load the configuration file
  QString confFile = "/usr/local/etc/pcdm.conf";
  if(!QFile::exists(confFile)){
    qDebug() << "PCDM: Configuration file missing:"<<confFile<<"\n  - Using default configuration";
    QFile::copy(":samples/pcdm.conf", confFile);
  }

  Config::loadConfigFile(confFile);
  // Now set the backend functionality of which usernames are allowed
  if(USERS==0){
    USERS = new UserList();
    USERS->allowUnder1K(Config::allowUnder1KUsers());
    USERS->allowRootUser(Config::allowRootUser());
    USERS->excludeUsers(Config::excludedUserList());
    USERS->updateList(); //now start the probe so things are ready on demand
  }
  //Backend::allowUidUnder1K(Config::allowUnder1KUsers(), Config::excludedUserList());
  //qDebug() << "Config File Loaded:" << QString::number(clock.elapsed())+" ms";
  // Startup the main application
  QApplication a(argc,argv);
    //Setup Log File
    qInstallMessageHandler(MessageOutput);
  int retCode = 0; //used for UI/application return
  // Show our splash screen, so the user doesn't freak that that it takes a few seconds to show up
  /*QSplashScreen splash;
  if(!Config::splashscreen().isEmpty()){
    splash.setPixmap( QPixmap(Config::splashscreen()) ); //load the splashscreen file
  }
  splash.show();*/
  //QCoreApplication::processEvents(); //Process the splash screen event immediately
  //qDebug() << "SplashScreen Started:" << QString::number(clock.elapsed())+" ms";
  //Initialize the xprocess
  XProcess desktop;

  // Check what directory our app is in
    QString appDir = "/usr/local/share/PCDM";
    // Load the translator
    QTranslator translator;
    QString langCode = lang;
    //Check for a language change detected
    //if ( ! changeLang.isEmpty() )
       //langCode = changeLang;
    //Load the proper locale for the translator
    if ( QFile::exists(appDir + "/i18n/PCDM_" + langCode + ".qm" ) ) {
      translator.load( QString("PCDM_") + langCode, appDir + "/i18n/" );
      a.installTranslator(&translator);
      qDebug() <<"Loaded Translation:" + appDir + "/i18n/PCDM_" + langCode + ".qm";
    } else {
      qDebug() << "Could not find: " + appDir + "/i18n/PCDM_" + langCode + ".qm";
      langCode = "en_US"; //always default to US english
    }


    QTextCodec::setCodecForLocale( QTextCodec::codecForName("UTF-8") ); //Force Utf-8 compliance
    //qDebug() << "Translation Finished:" << QString::number(clock.elapsed())+" ms";

  QProcess compositor;
  if ( QFile::exists("/usr/local/bin/compton") ) {
    compositor.start("/usr/local/bin/compton");
  }

  // Check if VirtualGL is in use, open up the system for GL rendering if so
  if ( Config::enableVGL() ) {
    system("xhost +");
  }

  // *** STARTUP THE PROGRAM ***
  bool goodAL = false; //Flag for whether the autologin was successful
  // Start the autologin procedure if applicable
  if( ALtriggered && Config::useAutoLogin() ){
    //Setup the Auto Login
    QString user = Backend::getALUsername();
    QString pwd = Backend::getALPassword();
    QString dsk = Backend::getLastDE(user, "");
    if( user.isEmpty() || dsk.isEmpty() || QFile::exists("/var/db/personacrypt/"+user+".key") ){
	//Invalid inputs (or a PersonaCrypt user)
	 goodAL=false;
    }else{
	//Run the time delay for the autologin attempt
	if(Config::autoLoginDelay() >= 0){
	  ThemeStruct ctheme;
             ctheme.loadThemeFile(Config::themeFile());
	  loginDelay dlg(Config::autoLoginDelay(), user, ctheme.itemValue("background") );
	  //splash.close();
	  dlg.start();
	    dlg.activateWindow();
	  dlg.exec();
	  goodAL = dlg.continueLogin;
	}else{
	  goodAL = true;
	}
	//now start the autologin if appropriate
	if(goodAL){
            //qDebug() << "Starting AutoLogin:" << user;
	  desktop.loginToXSession(user,pwd, dsk,lang,"",false);
	  //splash.close();
	  if(desktop.isRunning()){
	    goodAL=true; //flag this as a good login to skip the GUI
	  }
        }
    }
  }
  //qDebug() << "AutoLogin Finished:" << QString::number(clock.elapsed())+" ms";
  if(!goodAL){
    // ------START THE PCDM GUI-------
   retCode = -1;
   while(retCode==-1){
    retCode = 0;
    qDebug() << "Starting up PCDM interface";
    PCDMgui w;

    QLocale locale(lang); //Always use the "lang" saved from last login - even if the "langCode" was reset to en_US for loading PCDM translations
    w.setLocale(locale);
    //qDebug() << "Main GUI Created:" << QString::number(clock.elapsed())+" ms";
    //splash.finish(&w); //close the splash when the GUI starts up

    //Setup the signals/slots to startup the desktop session
    QObject::connect( &w,SIGNAL(xLoginAttempt(QString,QString,QString,QString, QString, bool)), &desktop,SLOT(loginToXSession(QString,QString,QString,QString, QString,bool)) ); 
    //Setup the signals/slots for return information for the GUI
    QObject::connect( &desktop, SIGNAL(InvalidLogin()), &w, SLOT(slotLoginFailure()) );
    QObject::connect( &desktop, SIGNAL(started()), &w, SLOT(slotLoginSuccess()) );
    QObject::connect( &desktop, SIGNAL(ValidLogin()), &w, SLOT(slotLoginSuccess()) );

    //qDebug() << "Showing GUI:" << QString::number(clock.elapsed())+" ms";
    w.show();
    //Set the flag which says that X started properly
    if( !QFile::exists("/var/tmp/.pcdm-x-started-"+VT) ){
      QProcess::startDetached("touch /var/tmp/.pcdm-x-started-"+VT);
    }
    //Now start the event loop until the window closes
    retCode = a.exec();
   }//end special retCode==-1 check (restart GUI)
  }  // end of PCDM GUI running
  USERS->stopUpdates();
  //Wait for the desktop session to finish before exiting
  if(compositor.state()==QProcess::Running){ compositor.terminate(); }
    desktop.waitForSessionClosed();
    qDebug() << "PCDM Session finished";
    logfile.close();
  //splash.show(); //show the splash screen again
  //Now wait a couple seconds for things to settle
  QTime wTime = QTime::currentTime().addSecs(2);
  while( QTime::currentTime() < wTime ){
    QCoreApplication::processEvents(QEventLoop::AllEvents,100);
  }
  //check for shutdown process
  if( QFile::exists(TMPSTOPFILE.arg(VT)) || QFile::exists("/var/run/nologin") || retCode > 0){
    //splash.showMessage(QObject::tr("System Shutting Down"), Qt::AlignHCenter | Qt::AlignBottom, Qt::white);
    QCoreApplication::processEvents();
    //Pause for a few seconds to prevent starting a new session during a shutdown
    wTime = QTime::currentTime().addSecs(30);
    while( QTime::currentTime() < wTime ){
      //Keep processing events during the wait (for splashscreen)
      QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
  }


  //Clean up Code
  delete &desktop;
  delete &a;
  //delete &splash;

  return retCode;
}

int main(int argc, char *argv[])
{
 bool neverquit = true;
 bool runonce = true;  //Always set this for now until the internal repeater works properly
 setenv("MM_CHARSET", "UTF-8", 1); //ensure UTF-8 text formatting
 VT = "vt9";
 for(int i=1; i<argc; i++){
    if(QString(argv[i]) == "-once"){ runonce = true; }
    else{  VT = QString(argv[i]); }
 }
 while(neverquit){
  if(runonce){ neverquit = false; }
  qDebug() << " -- PCDM Session Starting...";
  /*int sid = -1;
  int pid = fork();
  if(pid < 0){
    qDebug() << "Error: Could not fork the PCDM session";
    return -1;
  }else if( pid ==0 ){
    //New Child Process
    sid = setsid(); //start a session
    qDebug() << "-- Session ID:" << sid;*/
    int retCode = runSingleSession(argc,argv);
    qDebug() << "-- PCDM Session Ended --";
    //check for special exit code
    if(retCode == -1){ neverquit=true; } //make sure we go around again at least once
    else if(retCode != 0){ neverquit=false; }
    //Now kill the shild process (whole session)
    /*qDebug() << "Exiting child process";
    exit(3);
  }else{
    //Parent (calling) process
    int status;
    sleep(2);
    waitpid(sid,&status,0); //wait for the child (session) to finish
    //NOTE: the parent will eventually become a login-watcher daemon process that
    //   can spawn multiple child sessions on different TTY displays
  }*/
  qDebug() << "-- PCDM Session Ended --";
  if(QFile::exists("/var/run/nologin") || QFile::exists(TMPSTOPFILE.arg(VT)) ){ neverquit = false; }
 }
 return 0;
}
