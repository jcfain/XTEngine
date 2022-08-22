#include "xtengine.h"

XTEngine::XTEngine(QObject* parent) : QObject(parent)
{
    QCoreApplication::setOrganizationName("cUrbSide prOd");
    QCoreApplication::setApplicationName("XTEngine");

    SettingsHandler::Load();

    _tcodeFactory = new TCodeFactory(0.0, 1.0, this);
    _tcodeHandler = new TCodeHandler(this);
    _settingsActionHandler = new SettingsActionHandler(this);
    _mediaLibraryHandler = new MediaLibraryHandler(this);
    _connectionHandler = new ConnectionHandler(this);
    _syncHandler = new SyncHandler(this);
    if(SettingsHandler::getEnableHttpServer())
    {
        _httpHandler = new HttpHandler(_mediaLibraryHandler, this);
    }
}

void XTEngine::init()
{
    if(SettingsHandler::getEnableHttpServer())
    {
        connect(_httpHandler, &HttpHandler::tcode, _connectionHandler, &ConnectionHandler::sendTCode);
        connect(_httpHandler, &HttpHandler::xtpWebPacketRecieve, _connectionHandler, &ConnectionHandler::inputMessageSend);
        connect(_connectionHandler, &ConnectionHandler::connectionChange, _httpHandler, &HttpHandler::on_DeviceConnection_StateChange);

        _httpHandler->listen();
    }

    connect(_connectionHandler, &ConnectionHandler::messageRecieved, _syncHandler, &SyncHandler::searchForFunscript);
    connect(_connectionHandler, &ConnectionHandler::gamepadAction, _settingsActionHandler, &SettingsActionHandler::media_action);
    connect(_syncHandler, &SyncHandler::sendTCode, _connectionHandler, &ConnectionHandler::sendTCode, Qt::QueuedConnection);
    connect(_settingsActionHandler, &SettingsActionHandler::tcode_action, this, [this](QString tcode) {
        if(tcode == "DHOME") {
            tcode = _tcodeHandler->getAllHome();
        }
        _connectionHandler->sendTCode(tcode);
    });
    _connectionHandler->init();

    _mediaLibraryHandler->loadLibraryAsync();
}

XTEngine::~XTEngine() {
    _syncHandler->stopAll();
}
