#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

#include <song.h>

//数据库
//1.初始化和释放数据库链接
//2.建立一个歌曲表，提供歌曲表的读、写（数据库常规操作：增删改查）
//1）歌曲表songs （id , url, name, artist, album)

class DatabaseManager
{
public:
    static DatabaseManager* getInstance()
    {
        static DatabaseManager manager;
        return &manager;
    }

    bool init();

    void destroy();

    bool initSong();

    bool addSong(const Song& song);

    bool querySongs(QList<Song*>& songsResult);

    bool clearSongs();
private:
    QString m_databaseName = "F:/cQT/project/music/\\music.db";
    QSqlDatabase m_sqlDatabase;
    DatabaseManager()
    {
        m_sqlDatabase = QSqlDatabase::addDatabase("QSQLITE");
    }

};

#endif // DATABASEMANAGER_H
