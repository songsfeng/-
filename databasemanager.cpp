#include "databasemanager.h"

//初始化函数
//1.设置数据库名
//2. 打开数据库
//3. 初始化歌曲列表
bool DatabaseManager::init()
{
    m_sqlDatabase.setDatabaseName(m_databaseName);
    bool ret = m_sqlDatabase.open();
    if(ret)
    {
        ret = initSong();
        if(!ret)
        {
            qDebug() << "lnit songs table failed";
            destroy();
        }
    }
    else
    {
        qDebug() << "open database failed";
    }
    return ret;
}

//释放数据库
//1.如果数据库是打开状态就关闭
//2.移动数据库
void DatabaseManager::destroy()
{
    if(m_sqlDatabase.isOpen())
    {
        m_sqlDatabase.close();
    }

    QSqlDatabase::removeDatabase(m_databaseName);
}

//初始化歌曲表
//1.如果数据库是打开状态，就使用创建表的语句创建歌曲表songs
//歌曲表字段：id url name artist album
bool DatabaseManager::initSong()
{
    bool ret = m_sqlDatabase.isOpen();
    if(ret)
    {
        QSqlQuery query;
        query.prepare("create table if not exists songs ("
                       "id integer primary key,"
                       "url text not null,"
                        "name text not null UNIQUE,"
                        "artist text not null,"
                         "album text not null);");
        ret = query.exec();
        if(!ret)
        {
            qDebug() << "Init songs table failed,error:" << query.lastError().text();
        }

    }
    else
    {
        qDebug() << "Init songs table failed, sql is not open";
    }
    return ret;
}

//增加歌曲
//参数：song 要添加到数据库表里songs里的歌曲对象
//如果数据库是打开状态
bool DatabaseManager::addSong(const Song& song)
{
    bool ret = m_sqlDatabase.isOpen();
    if(ret)
    {
        QSqlQuery query;
        query.prepare("insert into songs(url,name,artist,album)"
                      "values(:url,:name,:artist,:album);");
        query.bindValue(":url",song.url());
        query.bindValue(":name",song.name());
        query.bindValue(":artist",song.artist());
        query.bindValue(":album",song.album());
        ret = query.exec();
        if(!ret)
        {
            qDebug() << "Add song failed,error:" << query.lastError().text();
        }
    }
    else
    {
        qDebug() << "Add song failed,sql is not open";
    }
    return ret;
}

//查询歌曲
//1.如果数据库是打开状态，使用查询语句select查询歌曲表中的歌曲，
//获取歌曲信息，构造一个歌曲对象
bool DatabaseManager::querySongs(QList<Song*>& songsResult)
{
    bool ret = m_sqlDatabase.isOpen();
    if(ret)
    {
        QSqlQuery query;
        query.prepare("select url,name,artist,album from songs;");
        QString url, name, artist, album;
        ret = query.exec();
        if(ret)
        {
            Song* song = nullptr;
            while (query.next())
            {
                url = query.value(0).toString();
                name = query.value(1).toString();
                artist = query.value(2).toString();
                album = query.value(3).toString();
                song = new Song(url, name, artist, album);
                songsResult.push_back(song);

            }
        }
        else
        {
            qDebug()<< "Query songs failed,error:" << query.lastError().text();
        }
    }
    else
    {
        qDebug() << "Query songs failed, sql is not open";
    }
    return ret;
}

//清空/删除歌曲表
//如果数据库是打开状态，就执行delete语句删除歌曲表songs
bool DatabaseManager::clearSongs()
{
    bool ret = m_sqlDatabase.isOpen();
    if(ret)
    {
        QSqlQuery query;
        query.prepare("delete from songs");
        ret = query.exec();
        if(!ret)
        {
            qDebug() << "clear songs failed,error:" << query.lastError().text();
        }
    }
    else
    {
        qDebug()<< "clear songs failed,sql is not open";
    }
    return ret;
}
