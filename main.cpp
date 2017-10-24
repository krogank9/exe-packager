#include <QApplication>
#include <QFileDialog>
#include <QString>
#include <QDebug>
#include <QStringList>
#include "miniz.h"

#ifdef _WIN32
    #define TMPDIR "%USERPROFILE%\\\\AppData\\\\Local\\\\Temp\\\\"
    #define SLASH "\\\\"
#else
    #define TMPDIR "/tmp/"
    #define SLASH "/"
#endif

QByteArray compressQBytes(QByteArray bytes) {
    QByteArray out(compressBound(bytes.size()), 0);
    unsigned long outLen = (unsigned long) out.size();
    compress((unsigned char*)out.data(), &outLen, (unsigned char*)bytes.data(), bytes.size());
    out.resize(outLen);
    return out;
}

QStringList fileNames;
QStringList fileContentsB64;
QStringList subDirsToMake;
QStringList fileSizesCompressed;
QStringList fileSizesUncompressed;

bool firstRecur = false;
bool recurseThroughDirs(QDir dir) {
    if(!firstRecur) {
        subDirsToMake.append(dir.absolutePath());
        firstRecur = false;
    }
    QFileInfoList fileInfoList = dir.entryInfoList();
    for(int i=0; i<fileInfoList.length(); i++) {
        QFileInfo info = fileInfoList.at(i);
        if(info.isDir()) {
            QDir recurDir(info.absoluteFilePath());
            if(recurDir.absolutePath() != dir.absolutePath()
            && recurDir.absolutePath().startsWith(dir.absolutePath())) {
                recurseThroughDirs(recurDir);
            }
        }
        else if(info.isFile()) {
            QString name = info.absoluteFilePath();
            QFile file(name);
            if (!file.open(QIODevice::ReadOnly))
                return false;
            QByteArray bytes = file.readAll();
            QByteArray compressedBytes = compressQBytes(bytes);
            fileSizesUncompressed.append( QString::number(bytes.size()) );
            fileSizesCompressed.append( QString::number(compressedBytes.size()) );
            fileContentsB64.append( compressedBytes.toBase64() );
            fileNames.append(name);
        }
    }
    return true;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString exePath = QFileDialog::getOpenFileName(0, "Select EXE to pack");

    if(exePath.length() == 0) {
        return 0;
    }

    // Exe selected, save name & recurse through its directory tree converting to base64.
    QFile exeFile(exePath);
    QFileInfo exeInfo(exeFile);
    QString exeName = exeInfo.fileName();

    QDir exeDir = exeInfo.absoluteDir();
    if( !recurseThroughDirs(exeDir) )
        return 0;

    QString exeShortName = exeInfo.baseName();

    // Make paths relative to tmp folder
    QString dirName = exeDir.absolutePath();
    for(int i=0; i<fileNames.length(); i++)
        fileNames[i] = TMPDIR+exeShortName+SLASH+fileNames[i].remove(dirName+"/").replace("/",SLASH);
    for(int i=0; i<subDirsToMake.length(); i++)
        subDirsToMake[i] = TMPDIR+exeShortName+subDirsToMake[i].remove(dirName).replace("/",SLASH);
    qInfo() << subDirsToMake;

    QString exeLoc = "char *exeName = \""+exeName+"\";";
    QString baseLoc = ("char *baseDir = \"" TMPDIR)+exeShortName+SLASH+"\";";
    QString dirsNum = "int numDirs = "+QString::number(subDirsToMake.length())+";";
    QString fileCount = "int fileCount = "+QString::number(fileNames.length())+";";
    QString fileSizesC = "int fileSizesCompressed[] = {"+fileSizesCompressed.join(", ")+"};";
    QString fileSizesU = "int fileSizesUncompressed[] = {"+fileSizesUncompressed.join(", ")+"};";

    QString list0 = "char *dirsToMake[] = {\""+subDirsToMake.join("\", \"")+"\"};";

    QString list1 = "char *fileNames[] = {\n\""+fileNames.join("\",\n\"")+"\"\n};";
    QString list2 = "static const char *fileContentsB64[] = {\n\""+fileContentsB64.join("\",\n\"")+"\"\n};";

    QFile unpackerC(":/unpacker.c");
    QFile minizH(":/miniz.h");
    if(!unpackerC.open(QIODevice::ReadOnly) || !minizH.open(QIODevice::ReadOnly))
        return 1;

    QString saveName = QFileDialog::getSaveFileName(0, "Choose filename to save as");
    QFile saveFile(saveName);
    saveFile.open(QIODevice::WriteOnly);

    QString toWrite = (QStringList {dirsNum, fileCount, fileSizesC, fileSizesU, list0, "\n", list1, list2, "\n", exeLoc, baseLoc}).join("\n") + "\n";
    saveFile.write(minizH.readAll() + "\n" + toWrite.toUtf8() + unpackerC.readAll());
}
