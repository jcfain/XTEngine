#include "outputdevicehandler.h"

OutputDeviceHandler::OutputDeviceHandler(QObject *parent) :
    QThread(parent)
{

}

OutputDeviceHandler::~OutputDeviceHandler()
{
    //wait();
}

void OutputDeviceHandler::run(){}

void OutputDeviceHandler::sendTCode(const QString &tcode)
{

}

void OutputDeviceHandler::dispose()
{

}

bool OutputDeviceHandler::isConnected()
{

}
