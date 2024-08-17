#ifndef TCODEVERSION_H
#define TCODEVERSION_H
#include <QObject>
#include <QDataStream>
enum TCodeVersion {
    v2,
    v3,
    v4
};

// QString operator+(const TCodeVersion& v) {
//     switch (v) {
//     case TCodeVersion::v2: return "V2";
//     case TCodeVersion::v3: return "V3";
//     }
//     return "";
// }
// QString& operator+(QString& string, const TCodeVersion& v) {
//     switch (v) {
//         case TCodeVersion::v2: string + "V2"; break;
//         case TCodeVersion::v3: string + "V3"; break;
//     }
//     return string;
// }
// QDataStream& operator<<(QDataStream& os, const TCodeVersion& v) {
//     switch (v) {
//         case TCodeVersion::v2: os << "V2"; break;
//         case TCodeVersion::v3: os << "V3"; break;
//         default: os.setStatus(QDataStream::Status::WriteFailed);
//     }
//     return os;
// }
Q_DECLARE_METATYPE(TCodeVersion);
#endif // TCODEVERSION_H
