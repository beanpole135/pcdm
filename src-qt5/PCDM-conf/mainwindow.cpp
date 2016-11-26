/**************************************************************************
*   Copyright (C) 2012 by Yuri Momotyuk                                   *
*   yurkis@gmail.com                                                      *
*   Copyright (C) 2013 by Kris Moore                                      *
*   kris@pcbsd.org                                                        *
*                                                                         *
*   Permission is hereby granted, free of charge, to any person obtaining *
*   a copy of this software and associated documentation files (the       *
*   "Software"), to deal in the Software without restriction, including   *
*   without limitation the rights to use, copy, modify, merge, publish,   *
*   distribute, sublicense, and/or sell copies of the Software, and to    *
*   permit persons to whom the Software is furnished to do so, subject to *
*   the following conditions:                                             *
*                                                                         *
*   The above copyright notice and this permission notice shall be        *
*   included in all copies or substantial portions of the Software.       *
*                                                                         *
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       *
*   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    *
*   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*
*   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR     *
*   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, *
*   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR *
*   OTHER DEALINGS IN THE SOFTWARE.                                       *
***************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QProcess>
#include <QProcessEnvironment>
#include <QFile>
#include <QTextStream>
#include <QDir>

#define DM_CONFIG_FILE QString("/usr/local/etc/pcdm.conf")

///////////////////////////////////////////////////////////////////////////////
inline bool toBoolean(QString Str, bool Default=false)
{
    if (Str.trimmed().toLower() == "true")
        return true;
    if (Str.trimmed().toLower() == "false")
        return false;
    return Default;
}

///////////////////////////////////////////////////////////////////////////////
inline QString fromBoolean(bool flag)
{
    return (flag)?QString("true"):QString("false");
}

///////////////////////////////////////////////////////////////////////////////
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    exUserMenu = new QMenu(this);
      ui->tool_exuser_add->setMenu(exUserMenu);
      connect(exUserMenu, SIGNAL(triggered(QAction*)), this, SLOT(add_exuser(QAction*)) );
    initUI();
}

///////////////////////////////////////////////////////////////////////////////
MainWindow::~MainWindow()
{
    delete ui;
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::initUI()
{

    //Make sure the conf file exists
    if(!QFile::exists(DM_CONFIG_FILE)){
      qDebug() << "Copying over the default configuration file:" << DM_CONFIG_FILE;
      if( !QFile::copy(DM_CONFIG_FILE+".dist", DM_CONFIG_FILE) ){
	QMessageBox::warning(this,tr("Missing Config File"), QString(tr("The PCDM configuration file could not be found: %1")).arg(DM_CONFIG_FILE)+"\n\n"+tr("This application will now close"));
	exit(1);
      }
    }
    getUsers();
    ui->UsersList->clear();
    QString autoLogDelay = getValFromSHFile(DM_CONFIG_FILE, "AUTO_LOGIN_DELAY");
    if(!autoLogDelay.isEmpty()){
      ui->spin_autoLogDelay->setValue(autoLogDelay.toInt());
    }
    
    QString autoLogUser = getValFromSHFile(DM_CONFIG_FILE, "AUTO_LOGIN_USER");
    for (int  i=0; i<mvUsers.size(); i++)
    {
        ui->UsersList->addItem(mvUsers[i]);
	if ( autoLogUser == mvUsers[i] )
          ui->UsersList->setCurrentIndex(i);
    }

    QString autoLog = getValFromSHFile(DM_CONFIG_FILE, "ENABLE_AUTO_LOGIN");
    if ( autoLog == "TRUE" ) {
      ui->AutoLoginEnabledCB->setChecked(true);
      ui->UsersList->setEnabled(true);
    } else {
      ui->AutoLoginEnabledCB->setChecked(false);
      ui->UsersList->setEnabled(false);
    }

    ui->EnableVNC->setChecked(false);
    QString vnc = getValFromSHFile(DM_CONFIG_FILE, "ALLOW_REMOTE_LOGIN");
    if ( vnc == "TRUE" )
      ui->EnableVNC->setChecked(true);

    ui->checkShowPW->setChecked(false);
    QString showpw = getValFromSHFile(DM_CONFIG_FILE, "ENABLE_VIEW_PASSWORD_BUTTON");
    if ( showpw == "TRUE" )
      ui->checkShowPW->setChecked(true);

    ui->checkShowUsers->setChecked(true); //PCDM defaults to true
    QString showusers = getValFromSHFile(DM_CONFIG_FILE, "SHOW_SYSTEM_USERS");
    if( showusers != "TRUE" && !showusers.isEmpty() ){
      ui->checkShowUsers->setChecked(false);
    }
    
    ui->checkAllowStealth->setChecked(false); //PCDM defaults to false
    QString allowstealth = getValFromSHFile(DM_CONFIG_FILE, "ALLOW_STEALTH_LOGIN");
    if(allowstealth.toLower() == "true"){
      ui->checkAllowStealth->setChecked(true);
    }
    
    ui->groupAllowUnder1K->setChecked(false); //PCDM defaults to false
    QString allowU1K = getValFromSHFile(DM_CONFIG_FILE, "ALLOW_UID_UNDER_1K");
    if(allowU1K.toLower() == "true"){
      ui->groupAllowUnder1K->setChecked(true);
    }   
    //Theme setting
    loadAvailableThemes(); //update the combobox first
    QString theme = getValFromSHFile(DM_CONFIG_FILE, "THEME_FILE");
    int index = ui->combo_themes->findData(theme);
    if(index>=0){ ui->combo_themes->setCurrentIndex(index);  ui->radio_theme_bundle->setChecked(true); }
    else{ ui->line_theme_custom->setText(theme); ui->radio_theme_custom->setChecked(true); }

    //Update the UI appropriately
    itemChanged();
    ui->SaveButton->setEnabled(false); //re-disable the save button because nothing has changed yet
    


    //Now setup all the signals/slots for updating the UI appropriately
    connect( ui->AutoLoginEnabledCB, SIGNAL(stateChanged(int)), this, SLOT(itemChanged()) );
    connect( ui->UsersList, SIGNAL(currentIndexChanged(int)), this, SLOT(itemChanged()) );
    connect( ui->spin_autoLogDelay, SIGNAL(valueChanged(int)), this, SLOT(itemChanged()) );
    connect( ui->EnableVNC, SIGNAL(stateChanged(int)), this, SLOT(itemChanged()) );
    connect( ui->checkShowPW, SIGNAL(stateChanged(int)), this, SLOT(itemChanged()) );
    connect( ui->checkShowUsers, SIGNAL(stateChanged(int)), this, SLOT(itemChanged()) );
    connect( ui->checkAllowStealth, SIGNAL(stateChanged(int)), this, SLOT(itemChanged()) );
    connect( ui->groupAllowUnder1K, SIGNAL(toggled(bool)), this, SLOT(itemChanged()) );
    connect( ui->radio_theme_bundle, SIGNAL(toggled(bool)), this, SLOT(itemChanged()) );
    connect( ui->radio_theme_custom, SIGNAL(toggled(bool)), this, SLOT(itemChanged()) );
    connect( ui->combo_themes, SIGNAL(currentIndexChanged(int)), this, SLOT(itemChanged()) );
    connect( ui->line_exuser, SIGNAL(returnPressed()), this, SLOT(on_tool_exuser_add_clicked()) );
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::getUsers()
{
    QStringList users = runShellCommand("getent passwd");
    mvUsers.clear();
    exUserMenu->clear();
    for(int i=0; i<users.length(); i++){
      QString line = users[i];
      if ((line.trimmed().indexOf("#") != 0) && (! line.isEmpty())) { //Make sure it isn't a comment or blank
	QString username = line.section(":",0,0);
	QString homedir = line.section(":",5,5);
	QString shell = line.section(":",6,6);
	//Ignore invalid users
	if(shell.contains("nologin") || homedir.contains("nonexistent") || homedir.endsWith("/empty") ){ continue; }
	
	// Ignore PEFS encrypted users
	if ( QFile::exists(homedir + "/.pefs.db") )
	  continue;

	int uid = line.section(":",2,2).toInt();
	if(uid==0){ continue; }
	if ((uid>1000)&&(uid<65534)){
	  mvUsers.push_back(username);
        }else if(QFile::exists(homedir) && QFile::exists(shell)){
	  exUserMenu->addAction(username);
	}
      }
    }
	
    /*QFile userFile("/etc/passwd");
    if ( userFile.open(QIODevice::ReadOnly) ) {
        QTextStream stream(&userFile);
        stream.setCodec("UTF-8");
        QString line;

        while ( !stream.atEnd() ) {
            line = stream.readLine();

            if ((line.trimmed().indexOf("#") != 0) && (! line.isEmpty())) { //Make sure it isn't a comment or blank
                QString username = line.section(":",0,0);
                QString homedir = line.section(":",5,5);

		// Ignore PEFS encrypted users
		if ( QFile::exists(homedir + "/.pefs.db") )
		  continue;

                int uid = line.section(":",2,2).toInt();
                if ((uid>1000)&&(uid<65534))
                    mvUsers.push_back(username);
            }//if gout user
        }//for all lines
    }//if can open file*/
}

///////////////////////////////////////////////////////////////////////////////
QString MainWindow::getConfFileValue( QString oFile, QString Key )
{
   return getConfFileValue(oFile, Key, 1);
}

QString MainWindow::getConfFileValue( QString oFile, QString Key, int occur )
{
	int found = 1;
    
    	QFile file( oFile );
    	if ( ! file.open( QIODevice::ReadOnly ) ) {
		return QString();
	}

       	QTextStream stream( &file );
       	QString line;
       	while ( !stream.atEnd() ) {
            	line = stream.readLine(); // line of text excluding '\n'
		line = line.section("#",0,0).trimmed(); //remove any comments
		if(line.isEmpty()){ continue; }
		int index = line.indexOf(Key, 0);
                //qDebug() << "Line:" << line << index;
                // If the KEY is not found at the start of the line, continue processing 
		if(index!=0)
			continue;
		
	    	if ( found == occur) {
 			line.remove(line.indexOf(Key, 0), Key.length());
    
    			// Remove any quotes
   			if ( line.indexOf('"') == 0 )
				line = line.remove(0, 1);
    
    			if ( line.indexOf('"') != -1  )
				line.truncate(line.indexOf('"'));

			file.close();
			
    			return line;
    		} else {
       			found++;  
    		}
        }

	file.close();
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
bool MainWindow::setConfFileValue( QString oFile, QString oldKey, QString newKey )
{
	return setConfFileValue(oFile, oldKey, newKey, -1);
}

bool MainWindow::setConfFileValue( QString oFile, QString oldKey, QString newKey, int occur )
{
    	// Lets the dev save a value into a specified config file. 
	// The occur value tells which occurance of "oldKey" to replace
    	// If occur is set to -1, it will remove any duplicates of "oldKey"
    
	//copy the original file to create a temporary file for editing
	QString oFileTmp = oFile + ".tmp";
	QString cmd = "cp "+oFile+" "+oFileTmp;
	runShellCommand(cmd);
	
	//Continue evaluating the temporary file
    	QStringList SavedFile;
    	int found = 1;

    	// Load the old file, find the oldKey, remove it and replace with newKey
    	QFile file( oFileTmp );
	if ( ! file.open( QIODevice::ReadOnly ) )
		return false;

        QTextStream stream( &file );
        QString line;
        while ( !stream.atEnd() ) {
        	line = stream.readLine(); // line of text excluding '\n'
            
		// Key is not found at all
		if ( line.indexOf(oldKey, 0) == -1 ) {
	        	SavedFile << line ;
			continue;
		}

		// Found the key, but it is commented out, so don't worry about this line
		if ( line.trimmed().indexOf("#", 0) == 0 ) {
	         	SavedFile << line ;
			continue;
		}

		// If the KEY is found, and we are just on wrong occurance, save it and continue to search  
		if ( occur != -1 && found != occur ) {
	         	SavedFile << line ;
			found++;
			continue;
		}

            	// If the KEY is found in the line and this matches the occurance that must be processed
            	if ( ! newKey.isEmpty() && found == occur )
           	{
	         	SavedFile << newKey ;
			newKey.clear();
	    		found++;
			continue;
            	} 

             	// If the KEY is found and we just want one occurance of the key
            	if ( occur == -1 && ! newKey.isEmpty() ) {
	         	SavedFile << newKey ;
			newKey.clear();
			found++;	
			continue;
		}

        }

   	file.close();

	// Didn't find the key? Write it!
	if ( ! newKey.isEmpty() )
	    SavedFile << newKey;
	
    
    	// Save the new file
    	QFile fileout( oFileTmp );
    	if ( ! fileout.open( QIODevice::WriteOnly ) )
		return false;

  	QTextStream streamout( &fileout );
	for (int i = 0; i < SavedFile.size(); ++i)
          	streamout << SavedFile.at(i) << "\n";

       	fileout.close();

	//Have the temporary file with new changes overwrite the original file
	cmd = "mv "+oFileTmp+" "+oFile;
	runShellCommand(cmd);	
	
	return true;
    
}

///////////////////////////////////////////////////////////////////////////////
QString MainWindow::getValFromSHFile(QString envFile, QString envVal)
{
  QFile confFile(envFile);
  if ( ! confFile.open( QIODevice::ReadOnly ) )
        return "";

  QTextStream stream( &confFile );
  QString line;
  while ( !stream.atEnd() ) {
     line = stream.readLine();
     if ( line.indexOf(envVal + "=") == 0 ) {
	line.replace(envVal + "=", "");

	// Remove the ' or " from variable
	if ( line.indexOf("'") == 0 ) {
	   line = line.section("'", 1, 1);
	   line = line.section("'", 0, 0);
	} 
	if ( line.indexOf('"') == 0 ) {
	   line = line.section('"', 1, 1);
	   line = line.section('"', 0, 0);
	}

        confFile.close();
	return line.simplified();
     }
  }
  confFile.close();

  return QString();
}

///////////////////////////////////////////////////////////////////////////////
QStringList MainWindow::runShellCommand(QString cmd){
   QProcess p;  
   //Make sure we use the system environment to properly read system variables, etc.
   p.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
   //Merge the output channels to retrieve all output possible
   p.setProcessChannelMode(QProcess::MergedChannels);   
   p.start(cmd);
   while(p.state()==QProcess::Starting || p.state() == QProcess::Running){
     p.waitForFinished(200);
     QCoreApplication::processEvents();
   }
   QString tmp = p.readAllStandardOutput();
    if(tmp.endsWith("\n")){ tmp.chop(1); }
    return tmp.split("\n");
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::loadAvailableThemes(){
  ui->combo_themes->clear();
  QDir themedir("/usr/local/share/PCDM/themes");
  QStringList dirs = themedir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
  QStringList themefiles;
  for(int i=0; i<dirs.length(); i++){
    themedir.cd(dirs[i]);
      QStringList files = themedir.entryList(QStringList() << "*.theme", QDir::Files, QDir::Name);
      for(int j=0; j<files.length(); j++){ themefiles << themedir.absoluteFilePath(files[j]); }
    themedir.cdUp();
  }
  //Now add the theme files to the combo box
  for(int i=0; i<themefiles.length(); i++){
    ui->combo_themes->addItem( themefiles[i].section("/",-1).section(".theme",0,-2), themefiles[i]);
  }
}
///////////////////////////////////////////////////////////////////////////////
void MainWindow::on_SaveButton_clicked()
{
    bool ok;
    system("touch " + DM_CONFIG_FILE.toLatin1());
    if ( ui->AutoLoginEnabledCB->isChecked() && !ui->UsersList->currentText().isEmpty() ) {
       // First ask for password
       QString pw = QInputDialog::getText(this, tr("Password Request"),
                        tr("Please enter the login password for this user"), 
                        QLineEdit::Password, "", &ok);
       if ( ! ok )
          return;
      
       setConfFileValue(DM_CONFIG_FILE, "ENABLE_AUTO_LOGIN", "ENABLE_AUTO_LOGIN=TRUE", -1);
       setConfFileValue(DM_CONFIG_FILE, "AUTO_LOGIN_USER", "AUTO_LOGIN_USER=" + ui->UsersList->currentText(), -1);
       setConfFileValue(DM_CONFIG_FILE, "AUTO_LOGIN_PASSWORD", "AUTO_LOGIN_PASSWORD=" + pw, -1);
       setConfFileValue(DM_CONFIG_FILE, "AUTO_LOGIN_DELAY","AUTO_LOGIN_DELAY="+QString::number(ui->spin_autoLogDelay->value()), -1);
    } else {
       ui->AutoLoginEnabledCB->setChecked(false); //make sure this is not checked to reflect file contents
       setConfFileValue(DM_CONFIG_FILE, "ENABLE_AUTO_LOGIN", "ENABLE_AUTO_LOGIN=FALSE", -1);
       setConfFileValue(DM_CONFIG_FILE, "AUTO_LOGIN_USER", "", -1);
       setConfFileValue(DM_CONFIG_FILE, "AUTO_LOGIN_PASSWORD", "", -1);
       setConfFileValue(DM_CONFIG_FILE, "AUTO_LOGIN_DELAY","", -1);
    }

    if ( ui->EnableVNC->isChecked() ) {
       // First ask for password
       QString pw = QInputDialog::getText(this, tr("Password Request"),
                        tr("Please enter the remote login password"), 
                        QLineEdit::Password, "", &ok);
       if ( ! ok )
          return;

       QFile file("/usr/local/etc/vncpass");
       if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
          return;

       QTextStream out(&file);
       out << pw;
       file.close();
       system("chmod 600 /usr/local/etc/vncpass");

       setConfFileValue(DM_CONFIG_FILE, "ALLOW_REMOTE_LOGIN", "ALLOW_REMOTE_LOGIN=TRUE", -1);
    } else {
       setConfFileValue(DM_CONFIG_FILE, "ALLOW_REMOTE_LOGIN", "ALLOW_REMOTE_LOGIN=FALSE", -1);
       system("rm /usr/local/etc/vncpass 2>/dev/null");
    }

    if ( ui->checkShowPW->isChecked() )
       setConfFileValue(DM_CONFIG_FILE, "ENABLE_VIEW_PASSWORD_BUTTON", "ENABLE_VIEW_PASSWORD_BUTTON=TRUE", -1);
    else
       setConfFileValue(DM_CONFIG_FILE, "ENABLE_VIEW_PASSWORD_BUTTON", "ENABLE_VIEW_PASSWORD_BUTTON=FALSE", -1);

    if(ui->checkShowUsers->isChecked()){
	setConfFileValue(DM_CONFIG_FILE, "SHOW_SYSTEM_USERS", "SHOW_SYSTEM_USERS=TRUE", -1);
    }else{
	setConfFileValue(DM_CONFIG_FILE, "SHOW_SYSTEM_USERS", "SHOW_SYSTEM_USERS=FALSE", -1);
    }
    if(ui->checkAllowStealth->isChecked()){
	setConfFileValue(DM_CONFIG_FILE, "ALLOW_STEALTH_LOGIN", "ALLOW_STEALTH_LOGIN=TRUE", -1);
    }else{
	setConfFileValue(DM_CONFIG_FILE, "ALLOW_STEALTH_LOGIN", "ALLOW_STEALTH_LOGIN=FALSE", -1);
    }
    if(ui->groupAllowUnder1K->isChecked()){
	setConfFileValue(DM_CONFIG_FILE, "ALLOW_UID_UNDER_1K", "ALLOW_UID_UNDER_1K=TRUE", -1);
    }else{
	setConfFileValue(DM_CONFIG_FILE, "ALLOW_UID_UNDER_1K", "ALLOW_UID_UNDER_1K=FALSE", -1);
    }
    QStringList exusers;
    for(int i=0; i<ui->list_exuser->count(); i++){
      exusers << ui->list_exuser->item(i)->text();
    }
    exusers.removeDuplicates();
    setConfFileValue(DM_CONFIG_FILE,"EXCLUDED_USERS", "EXCLUDED_USERS="+exusers.join(","), -1);
 
    QString theme;
    if(ui->radio_theme_bundle->isChecked()){
      theme = ui->combo_themes->currentData().toString();
    }else{
      theme = ui->line_theme_custom->text();
    }
    if(!theme.isEmpty()){ setConfFileValue(DM_CONFIG_FILE,"THEME_FILE", "THEME_FILE="+theme, -1); }

    // Lastly make sure we set perms
    system("chmod 600 " + DM_CONFIG_FILE.toLatin1());
    ui->SaveButton->setEnabled(false);
}


///////////////////////////////////////////////////////////////////////////////
void MainWindow::slotSingleInstance()
{
   this->hide();
   this->showNormal();
   this->activateWindow();
   this->raise();
}

/////////////////////////////////////////////////
void MainWindow::itemChanged(){
  ui->SaveButton->setEnabled(true);
  //Double check the dependant options for whether they are possible to be changed
  ui->UsersList->setEnabled( ui->AutoLoginEnabledCB->isChecked() );
  ui->spin_autoLogDelay->setEnabled( ui->AutoLoginEnabledCB->isChecked() );
}

void MainWindow::on_tool_exuser_rem_clicked(){
  QList<QListWidgetItem*> sel = ui->list_exuser->selectedItems();
  for(int i=0; i<sel.length(); i++){
    delete sel[i];
  }
  itemChanged();
}

void MainWindow::on_tool_exuser_add_clicked(){
  QString name = ui->line_exuser->text();
  ui->line_exuser->setText("");
  name = name.remove(" ");
  if(name.isEmpty() || !ui->list_exuser->findItems(name,Qt::MatchExactly).isEmpty() ){ return; }
  ui->list_exuser->addItem(name);
  itemChanged();
}

void MainWindow::add_exuser(QAction* act){ //for a username selected from the menu
  if( !ui->list_exuser->findItems(act->text(), Qt::MatchExactly).isEmpty() ){ return; }
  ui->list_exuser->addItem(act->text());
  itemChanged();
}
