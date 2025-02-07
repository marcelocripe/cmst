/**************************** counter.cpp ******************************** 

Code for the connection counter registered on DBus.  When registered the
connman daemon will communicate to this object with signals.

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

# include "./counter.h"
# include "../resource.h" 

//  header files generated by qmake from the xml file created by qdbuscpp2xml
# include "counter_adaptor.h"
# include "counter_interface.h"

//  constructor
ConnmanCounter::ConnmanCounter(QObject* parent)
    : QObject(parent)
{ 
  //  data members
  home_data = QVariantMap();
  roam_data = QVariantMap();
  
  //  Create Adaptor and register this Counter on the system bus.  
  new CounterAdaptor(this);
  
	// Try to register an object on the system bus
	QDBusConnection::systemBus().registerObject(CNTR_OBJECT, this);
	
}


/////////////////////////////////////// PUBLIC FUNCTIONS ////////////////////////////////
//
//  Function to return a QString for display in a label
//  map is the member map (home_data or roam_data) we wish to get
QString ConnmanCounter::getLabel(const QVariantMap& map)
{ 
  // Set TX bytes to Bytes, KB, MB, or GB depending on size
  const int b_cutoff = 1024 * 1.875       ; // size in Bytes to change units from Bytes to KB
  const int k_cutoff = 1024 * 1024 * 1.875 ; // size in Bytes to change units from KB to MB
  const int m_cutoff = 1024 * 1024 * 1024 * 1.875 ; // size in Bytes to change units from MB to GB
  QString datafield;
  if (map.value("TX.Bytes").toLongLong() < b_cutoff ) datafield = tr("%L1 Bytes").arg(map.value("TX.Bytes").toLongLong());
  else if (map.value("TX.Bytes").toLongLong() <  k_cutoff)                                                      
    datafield = tr("%L1 KB").arg(static_cast<double>(map.value("TX.Bytes").toLongLong()) / (1024), 0, 'f', 1);  
      else if (map.value("TX.Bytes").toLongLong() <  m_cutoff)                                                      
        datafield = tr("%L1 MB").arg(static_cast<double>(map.value("TX.Bytes").toLongLong()) / (1024 * 1024), 0, 'f', 1); 
          else 
            datafield = tr("%L1 GB").arg(static_cast<double>(map.value("TX.Bytes").toLongLong()) / (1024 * 1024 * 1024), 0, 'f', 1);  

  // Create a label with the total number of packets [errors and dropped] sent.
  QString rtn = tr("<b>Transmit:</b><br>TX Total: %1 (%2),  TX Errors: %3,  TX Dropped: %4")
                            .arg(tr("%Ln Packet(s)", 0, map.value("TX.Packets").toLongLong()) ) 
                            .arg(datafield)                                                   
                            .arg(tr("%Ln Packet(s)", 0, map.value("TX.Errors").toLongLong()) )  
                            .arg(tr("%Ln Packet(s)", 0, map.value("TX.Dropped").toLongLong()) ) ;


  // Set RX data bytes to Bytes, KB, MB or GB
  if (map.value("RX.Bytes").toLongLong() < b_cutoff ) datafield = tr("%L1 Bytes").arg(map.value("RX.Bytes").toLongLong());
  else if (map.value("RX.Bytes").toLongLong() <  k_cutoff)                                                      
    datafield = tr("%L1 KB").arg(static_cast<double>(map.value("RX.Bytes").toLongLong()) / (1024), 0, 'f', 1);  
      else if (map.value("RX.Bytes").toLongLong() <  m_cutoff)                                                      
        datafield = tr("%L1 MB").arg(static_cast<double>(map.value("RX.Bytes").toLongLong()) / (1024 * 1024), 0, 'f', 1); 
          else 
            datafield = tr("%L1 GB").arg(static_cast<double>(map.value("RX.Bytes").toLongLong()) / (1024 * 1024 * 1024), 0, 'f', 1);  

  // Append to the label the total number of packets [errors and dropped] received.
  rtn.append(tr("<br><br><b>Received:</b><br>RX Total: %1 (%2),  RX Errors: %3,  RX Dropped: %4")             
                              .arg(tr("%Ln Packet(s)", 0, map.value("RX.Packets").toLongLong()) )   
                              .arg(datafield)                                                     
                              .arg(tr("%Ln Packet(s)", 0, map.value("RX.Errors").toLongLong()) )  
                              .arg(tr("%Ln Packet(s)", 0, map.value("RX.Dropped").toLongLong())) );     
  
  // Append the time title
  rtn.append(tr("<br><br><b>Connect Time:</b><br>") );                                                                                                                                                                              
                                                                                                                                                                                                                                                      
  // Calculate the connection time and add it to the end of the label.
  int num_d = 0;
  short num_h = 0;
  short num_m = 0;
  short num_s = 0;
  
  int etime = map.value("Time").toInt();
  num_d = etime / (24 * 60 * 60);
  if (num_d > 0 ) {
    rtn.append(tr("%n Day(s)", 0, num_d) );
    rtn.append(" ");
  }
  etime = etime % (24 * 60 * 60); 
  
  num_h = etime / (60 * 60);
  if (num_h > 0 || num_d != 0) {
    rtn.append(tr("%n Hour(s)", 0, num_h) );
    rtn.append(" ");
  }
  etime = etime % (60 * 60);
  
  num_m = etime / (60);
  if (num_m > 0 || num_d != 0 || num_h != 0) {
    rtn.append(tr("%n Minute(s)", 0, num_m) );
    rtn.append(" ");
  }
  etime = etime % (60); 
  
  num_s = etime;
  if ( num_s > 0 || num_d != 0 || num_h != 0 || num_m != 0) rtn.append(tr("%n Second(s)", 0, num_s) );
  
  // Return the string
  return rtn;
}

/////////////////////////////////////// PUBLIC Q_SLOTS////////////////////////////////
//
// Called when the service daemon unregisters the counter.  QT deals with cleanup
// tasks so don't need much here
void ConnmanCounter::Release()
{
	//qDebug() << "Counter Released";
	
  return;
}

//
//  Called when there is a change in counter values
void ConnmanCounter::Usage(QDBusObjectPath qdb_objpath, QVariantMap home, QVariantMap roaming)
{
  // First time through connman will send home and roaming fully loaded.  After that only
  // items that change are sent.  We need to keep the data as a class member
  QMapIterator<QString, QVariant> i(home);
  while (i.hasNext()) {
    i.next();
    home_data[i.key()] = i.value();
    }
    
  QMapIterator<QString, QVariant> j(roaming);
  while (j.hasNext()) {
    j.next();
    roam_data[j.key()] = j.value();
    }    
  
  // Emit signal with object and labels to display
  emit usageUpdated(qdb_objpath, getLabel(home_data), getLabel(roam_data) );

  return;
}


