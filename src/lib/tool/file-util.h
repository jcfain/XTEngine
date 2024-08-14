#ifndef FILEUTIL_H
#define FILEUTIL_H
#include "QFile"
#include "QFileInfo"
#include "QString"
#include "QDirIterator"

#include "../handler/loghandler.h"

class XFileUtil
{
public:
    static QStringList getFunscriptPaths(const QString &videoPath, const QStringList &extensions, const QString& path)
    {
        QStringList funscripts;
        QString tempName = getNameNoExtension(videoPath);
        foreach (QString extension, extensions) {
            funscripts << tempName + extension;
        }

        QString separator = getSeperator(path);
        QStringList funscriptPaths;
        foreach (QString funscript, funscripts) {
            funscriptPaths << path + (path.endsWith(separator) ? "" : separator) + funscript;
        }

        return funscriptPaths;
    }

    static QString getNameNoExtension(const QString& file)
    {
        QFileInfo fileInfo(file);
        QString tempName = fileInfo.fileName();
        int indexOfSuffix = tempName.lastIndexOf(".");
        return indexOfSuffix > 0 ? tempName.remove(indexOfSuffix, tempName.length()) : tempName;
    }

    static QString getPathNoExtension(const QString& file)
    {
        int indexOfSuffix = file.lastIndexOf(".");
        return indexOfSuffix > 0 ? QString(file).remove(indexOfSuffix, file.length()) : file;
    }

    static QString searchForFileRecursive(const QString& videoPath, const QStringList &extensions, const QString& pathToSearch, const bool &cancel)
    {
        QString funscriptPath;
        LogHandler::Debug("searchForFileRecursive: Search ALL sub directories of: "+pathToSearch);
        QString nameNoExtension = getNameNoExtension(videoPath);
        if(!pathToSearch.isEmpty() && QFile::exists(pathToSearch))
        {
            QDirIterator directory(pathToSearch, QDirIterator::Subdirectories);
            while (directory.hasNext()) {
                if(cancel) {
                    return funscriptPath;
                }
                directory.next();
                if (QFileInfo(directory.filePath()).isFile()) {
                    //QString separator = getSeperator(directory.filePath());
                    QString fileName = directory.fileName();
                    foreach (QString extension, extensions) {
                        if (fileName.contains(nameNoExtension + extension)) {
                            funscriptPath = directory.filePath();
                            LogHandler::Debug("searchForFileRecursive File found: "+funscriptPath);
                            return funscriptPath;
                        }
                    }
                }
            }
        }
        return funscriptPath;
    }
    static QString getSeperator(const QString& ref)
    {
        return ref.contains("/") ? "/" : "\\";
    }
    static bool isDirParentChildOrEqual(const QStringList &originalValues, const QString &newValue, const QString &name, QStringList &messages) {
        foreach (auto originalPath, originalValues) {
            if(originalPath==newValue) {
                messages << "Directory '"+newValue+"' is already in the "+name+" list!";
            } else if(originalPath.startsWith(newValue)) {
                messages << "Directory '"+newValue+"'\nis a parent of\n'"+originalPath+"' in "+name;
            } else if(newValue.startsWith(originalPath)) {
                messages << "Directory '"+newValue+"'\nis a child of\n'"+originalPath+"' in "+name;
            }
        }
        return !messages.isEmpty();
    }
    static bool isDirParentOrEqual(const QStringList &originalValues, const QString &newValue, const QString &name, QStringList &messages) {
        foreach (auto originalPath, originalValues) {
            if(originalPath==newValue) {
                messages << "Directory '"+newValue+"' is already in the "+name+" list!";
            } else if(originalPath.startsWith(newValue)) {
                messages << "Directory '"+newValue+"'\nis a parent of\n'"+originalPath+"' in "+name;
            }
        }
        return !messages.isEmpty();
    }
    static bool isDirChildOrEqual(const QStringList &originalValues, const QString &newValue, const QString &name, QStringList &messages) {
        foreach (auto originalPath, originalValues) {
            if(originalPath==newValue) {
                messages << "Directory '"+newValue+"' is already in the "+name+" list!";
            } else if(newValue.startsWith(originalPath)) {
                messages << "Directory '"+newValue+"'\nis a child of\n'"+originalPath+"' in "+name;
            }
        }
        return !messages.isEmpty();
    }
};

#endif // FILEUTIL_H
