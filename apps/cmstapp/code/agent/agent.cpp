
/**************************** agent.cpp ******************************** 

Code for the user agent registered on DBus.  When the connman daemon
needs to communicate with the user it does so through this agent.

Copyright (C) 2013-2022
by: Andrew J. Bibb
License: MIT 

Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"),to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
copies of the Software, and to permit persons to whom the Software is 
furnished to do so, subject to the following conditions: 

The above copyright notice and this permission notice shall be included 
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.
***********************************************************************/  

# include <QtCore/QDebug>
# include <QtDBus/QDBusConnection>
# include <QMessageBox>
# include <QDialog>
# include <QFile>
# include <QTextStream>

# include "./agent.h"
# include "../resource.h" 
# include "./code/trstring/tr_strings.h"

//  header files generated by qmake from the xml file created by qdbuscpp2xml
# include "agent_adaptor.h"
# include "agent_interface.h"

//  defines
# define ERROR_RETRY "net.connman.Agent.Error.Retry"
# define ERROR_CANCELED "net.connman.Agent.Error.Canceled"
# define ERROR_LAUNCHBROWSER "net.connman.Agent.Error.LaunchBrowser"

//  constructor
ConnmanAgent::ConnmanAgent(QObject* parent)
    : QObject(parent)
{ 
  // members
  uiDialog = new AgentDialog(qobject_cast<QWidget *> (this) );
  input_map.clear();
  b_loginputrequest = false;
  
  //  Create Adaptor and register this Agent on the system bus.  
  new AgentAdaptor(this);
  QDBusConnection::systemBus().registerObject(AGENT_OBJECT, this);
  
}

/////////////////////////////////////// PUBLIC Q_SLOTS////////////////////////////////
//
// Called when the service daemon unregisters the agent.  QT deals with cleanup
// tasks so don't need much here
void ConnmanAgent::Release()
{
  //qDebug() << "Agent Released";
  
  return;
}

// Called when an error has to be reported to the user.  Show the
// error in a QMessageBox
void ConnmanAgent::ReportError(QDBusObjectPath path, QString s_error)
{
  (void) path;
  
  if ( QMessageBox::warning(qobject_cast<QWidget *> (parent()), tr("Connman Error"),
    tr("Connman returned the following error:<b><center>%1</b><br>Would you like to retry?").arg(TranslateStrings::cmtr(s_error)),
    QMessageBox::Yes | QMessageBox::No,
    QMessageBox::No
     ) == QMessageBox::Yes) this->sendErrorReply(ERROR_RETRY, "Going to retry the request");  
  
  else  return; 
}

//
// Called when it is required to ask the user to open a website to proceed
// with login handling
void ConnmanAgent::RequestBrowser(QDBusObjectPath path, QString url)
{
  (void) path;
  
  // Send the url to the dialog to have the user the necessary information, return if canceled. 
  if (this->uiDialog->showPage1(url) == QDialog::Rejected) this->sendErrorReply(ERROR_CANCELED,"User cancelled the dialog");
  
  return; 
}

//
// Called when trying to connect to a service and some extra input is required from the user
// A dialog is displayed with the required fields enabled (non-required fields are disabled).
QVariantMap ConnmanAgent::RequestInput(QDBusObjectPath path, QMap<QString,QVariant> dict)
{
  (void) path;
  
  // Take the dict returned by DBus and extract the information we are interested in and place in input_map.
  this->createInputMap(dict);
  
  // Send our input_map to the dialog to have the user supply the necessary information
  // needed to continue.  Return if canceled. 
  QMap<QString,QVariant> rtn;
  rtn.clear();
  if (this->uiDialog->showPage0(input_map) == QDialog::Rejected) 
   this->sendErrorReply(ERROR_CANCELED,"User cancelled the dialog");  
  else
    uiDialog->createDict(rtn);  // create a return dict and send it back to connman on DBus  
  
  return rtn;
}

//
// Called when the agent request failed before a reply was returned. Show
// a QMessageBox
void ConnmanAgent::Cancel()
{
  QMessageBox::information(qobject_cast<QWidget *> (parent()), tr("Agent Request Failed"),
    tr("The agent request failed before a reply was returned.") );    
    
  return; 
}

/////////////////////////////////////// PUBLIC FUNCTIONS ////////////////////////////////
//
//  Function to put all of input fields received via DBus:RequestInput into a 
//  QMap<QString,QString> where key is the input field received and value is
//  generally blank but can be used for informational text.
//
//  If we asked to log the input request create the log file in /tmp/cmst/input_request.log
void ConnmanAgent::createInputMap(const QMap<QString,QVariant>& r_map)
{
  // Initialize our data map
  input_map.clear();
  
  // QFile object for logging
  QTextStream log;
  QDir d(IPT_REQ_LOG_PATH);
  QFile logfile(d.absoluteFilePath(IPT_REQ_LOG_FILE));
  if (b_loginputrequest) {
    if (!logfile.open(QIODevice::WriteOnly | QIODevice::Text)) b_loginputrequest = false;
    else log.setDevice(&logfile);
  }
 
  
  // Run through the r_map getting the keys and the few values we are interested in.
  QMap<QString, QVariant>::const_iterator i = r_map.constBegin();
  while (i != r_map.constEnd()) {
        
    // Lets see what the values contain, but first make sure we can get to them.
    if (b_loginputrequest) log << "Agent: " << "Map Key = " << i.key() << "\n";
     
    if (! i.value().canConvert<QDBusArgument>() ) return;
    const QDBusArgument qdba =  i.value().value<QDBusArgument>();
    if (qdba.currentType() != QDBusArgument::MapType ) {
      if (b_loginputrequest) log << "Agent: " << "Error - QDBusArgument as the value is not a MapType\n"; 
      return;
    } 
    
    // The r_map.value() is a QDBusArgument::MapType so extract this map into a new QMap called m.
    qdba.beginMap();
    QMap<QString,QString> m;
    m.clear();
    if (b_loginputrequest) log << "Agent: " << "Extracting the DBusArgument Map...\n"; 
    while ( ! qdba.atEnd() ) {
      QString k;
      QVariant v;
      qdba.beginMapEntry();
      qdba >> k >> v;
      qdba.endMapEntry();
      m.insert(k, v.toString());
      if (b_loginputrequest) log << "{ " << k << " , " << v.toString() << "}\n"; 
    } // while
    qdba.endMap();
    
    // Browse through QMap m and get things we need to look at
    // Types we don' really care about.  We ignore "optional" and "alternate" requirements
    // and only extract the "mandatory" and "informational" requirements with values
    if (m.contains("Requirement") ) {
      QString val = QString();
      if ( m.value("Requirement").contains("mandatory", Qt::CaseInsensitive) || m.value("Requirement").contains("informational", Qt::CaseInsensitive) ) {
        if (m.contains("Value") ) val = m.value("Value"); 
      } // if mandatory or informational
      //  create our input_map entry
      input_map[i.key()] = val;
    } // if requirement

    ++i;
  } // while
  
  logfile.close();
  return; 
}

