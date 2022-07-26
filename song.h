#ifndef SONG_H
#define SONG_H

#include <QUrl>
#include <QString>
class Song
{
public:
    Song(){}
    Song(const QUrl url,const QString name,
         const QString artist,const QString album)
        :m_url(url),m_name(name),m_artist(artist),m_album(album){}
    const QUrl& url() const;
    const QString& name() const;
    const QString& artist() const;
    const QString& album() const;
private:
    QUrl m_url;//歌曲路径
    QString m_name;//歌曲名
    QString m_artist;//歌手名
    QString m_album;//专辑名
};

#endif // SONG_H
