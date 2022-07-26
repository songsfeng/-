#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
//添加多媒体播放头文件
#include <QMediaPlayer>//播放
#include <QMediaPlaylist>//播放列表
#include <QFileDialog>
#include <QTimer>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include <song.h>
#include <databasemanager.h>
namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();
    //界面相关设置
    void InterfaceSettings();
    //读取歌词文件
    void readLyrucsFromFile(const QString& lyricsFilePath);
    //显示当前歌曲歌词文件
    void updateLyricsAll();
    //同步歌词与进度条
    void updateLyricsOnTime();
    //打开数据库
    bool initDatabase();
    //查询所有歌曲，并添加到列表
    void getPlaylistFromSql();
    //添加到数据库
    void addSongToSqlPlaylist(const Song& song);
    //解析歌曲文件，m_url歌曲路径，m_name歌曲名，m_artist歌手名
    //m_album;专辑名
    void getSongInfoFromMp3File(const QString& filePath);
    //删除歌曲，调用数据库类
    void clearSqlPlaylist();
    //数据库释放
    void destroyDatabase();
    //判断是否与数据库里歌曲重复 查重
    bool JudgeSongRepetition(const QString& filePath);

private slots:
    //添加歌曲
    void on_add_clicked();
    //播放和暂停按钮
    void on_play_clicked();
    //停止
    void on_stop_clicked();
    //修改进度条，同步歌曲进度
    void on_progress_sliderReleased();
    //设置播放进度条与播放时间同步 显示当前播放时间和歌曲时长[00:00/00:/00]  播放器播放位置的变化
    void handlPlayerPositionChanged(qint64 position);
    //媒体音量同步
    void on_volume_sliderReleased();
    //转换歌曲路径获得歌词路径 .mp3 <---> .lrc
    void handlePlaylistCurrentMediaChanged(const QMediaContent&);
    //播放模式 切换
    void on_playbackMode_clicked();
    //播放模式图片切换
    void handlePlaylistPlaybackModeChanged(QMediaPlaylist::PlaybackMode mode);
    //上一首
    void on_previous_clicked();
    //下一首
    void on_next_clicked();
    //处理定时器超时信号的槽函数/每100毫秒刷新
    void handleTimeout();
    //播放器切换播放和暂停按钮图片    播放器切换播放和暂停
    void handlePlayerStateChanged(QMediaPlayer::State);
    //清空所有列表信息
    void on_clearPlaylist_clicked();
    //退出
    void on_quit_clicked();
    //播放双击选中的歌曲
    void on_playlist_doubleClicked(const QModelIndex &index);
    //播放双击选中的歌词
    void on_lyrics_doubleClicked(const QModelIndex &index);
    //静音功能
    void on_toolButton_clicked();

private:
    Ui::Widget *ui;
    //声明一个播放器成员变量
    QMediaPlayer *m_player;
    //声明一个播放列表成员变量
    QMediaPlaylist *m_playlist;
        //以多个键值对<qint64, QString>的形式保存歌词内容，
       //其中键是qint64整数类型的毫秒时间，
       //值是QString类型的歌词文本
    QMap<qint64,QString>m_lyrics;//声明一个容器
    QTimer* m_timer;    //声明一个定时器超时信号
    DatabaseManager* m_database;//声明一个数据库
    Song *song;    //声明一个歌曲信息类
    //声明一个map容器，储存歌曲文件路径，歌曲类song
    QMap<QString,Song*> m_currentPlaylist;
    QString VolumeTime;//记录音量

};

#endif // WIDGET_H
