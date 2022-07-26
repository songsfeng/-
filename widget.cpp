#include "widget.h"
#include "ui_widget.h"
#include <QMessageBox>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    this->setFixedSize(579,546);
    ui->setupUi(this);
    //分配一个播放器aa
    m_player = new QMediaPlayer;

    //分配一个播放列表
    m_playlist = new QMediaPlaylist;
    m_player->setPlaylist(m_playlist);
    m_playlist->setPlaybackMode(QMediaPlaylist::PlaybackMode::Sequential);

    //初始化界面
    InterfaceSettings();

    //链接，接收播放器的播放位置变化信号，并调用自定义槽函数处理
    connect(m_player,&QMediaPlayer::positionChanged,
            this,&Widget::handlPlayerPositionChanged);//显示播放时间
    connect(m_player,&QMediaPlayer::stateChanged,
            this,&Widget::handlePlayerStateChanged);//切换相应图标
    //链接，接受播放列表的信号，调用相关自定义槽函数
    connect(m_playlist, &QMediaPlaylist::currentMediaChanged,\
            this, &Widget::handlePlaylistCurrentMediaChanged);//切换歌词
    connect(m_playlist, &QMediaPlaylist::playbackModeChanged,\
            this, &Widget::handlePlaylistPlaybackModeChanged);//播放列表模式图标变换

    //定时器
    m_timer = new QTimer(this);
    connect(m_timer,&QTimer::timeout,this,&Widget::handleTimeout);
    m_timer->start(100);

    //数据库打开
    if(initDatabase())
    {
        getPlaylistFromSql();
    }
}

//释放
Widget::~Widget()
{   //释放m_currentPlaylist保存的Song*
    for (auto item : m_currentPlaylist)
    {
        delete item;
    }
    delete m_timer;//释放定时器
    delete m_player;//释放播放器
    delete m_playlist;//释放播放列表
    destroyDatabase();//释放数据库
    delete ui;
}

//界面相关设置
void Widget::InterfaceSettings()
{   //音量
    m_player->setVolume(50);
    ui->volumelabel->setText("50");
    ui->volume->setValue(50);
    //进度条去除 歌词框
    ui->lyrics->setHorizontalScrollBarPolicy //水平进度条去除
            (Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    ui->lyrics->setVerticalScrollBarPolicy  //垂直进度条去除
            (Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    ui->lyrics->setStyleSheet("background-color:transparent");//背景颜色透明
    ui->lyrics->setFrameShape(QListWidget::NoFrame);//去除边框
    //播放列表框
    ui->playlist->setHorizontalScrollBarPolicy
            (Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    ui->playlist->setVerticalScrollBarPolicy
            (Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    ui->playlist->setStyleSheet("background-color:transparent");
    ui->playlist->setFrameShape(QListWidget::NoFrame);





}


//描述：读取歌词文件
//逻辑：
//清处m_lirics容器内容
//读取歌词文件
//  按行读取
//  分割歌词    时间标签+歌词文本
//          时间标签        歌词文本
//      分钟    秒.毫秒     歌词文件
//      毫秒时间公式 = （分钟*60 + 秒.毫秒）*1000
//保存文件到容器m_lirics里  <qint64 时间标签，Qstring 歌词文本>
void Widget::readLyrucsFromFile(const QString &lyricsFilePath)
{
    //清除保存在M_LYRICS键值对容器MAP里上首歌的歌词
    m_lyrics.clear();
    //声明一个字符串保存每行的内容  时间标签+文本line
    QString line;
    //声明两个字符串保存第一二次分割，一次分割后内容为  时间标签|，文本
    //二次分割后内容为  分钟|，秒数.毫秒数
    QStringList lineContents , timeContents;

       //读取文件内容   实列化一个文件对象，通过传入的文件路径来读取
    QFile file(lyricsFilePath);
    //使用只读和读取文本的方式，读取文件
    if(file.open(QIODevice::ReadOnly | QIODevice::Text))//读取方式，和读取到N换行
    {
        QTextStream textStream(&file);
        while(!textStream.atEnd())
        {
            //按行读取
            line = textStream.readLine();

            if(!line.isEmpty())
            {
                //按字符 ] 进行分割，返回分割后的内容  时间标签  | 文本
                lineContents = line.split("]");
                //按字符 ：分割lineContents[0] 时间标签 返回分割后的内容 分钟 | 秒.毫秒
                timeContents = lineContents[0].split(":");
                //计算毫秒时间公式 = （分钟*60 +秒.毫秒）*1000;
                int minutes = timeContents[0].mid(1).toInt();
                double seconds = timeContents[1].toDouble();
                double time = minutes*60*1000 + seconds *1000;
                //保存文件内容到 m_lyrics容器中<一行时间标签，一行歌词的文本>
                m_lyrics.insert(time,lineContents[1]);
                lineContents.clear();
                timeContents.clear();

            }

        }

    }
}

//描述：显示所有歌词文件
//逻辑：
//读取歌词变量里的所有文本 m_lyrics<qint64 time,Qstring text>
//写入到界面里的歌词窗口
void Widget::updateLyricsAll()
{//清除上首歌词
    ui->lyrics->clear();

    if(!m_lyrics.empty())
    {

        for(auto text:m_lyrics.values())
        {

            QListWidgetItem *item = new QListWidgetItem();
            item->setText(text);
            item->setTextAlignment(Qt::AlignCenter);
            ui->lyrics->addItem(item);
        }

    }else{
        QListWidgetItem *item = new QListWidgetItem();
        item->setText("当前歌曲暂无歌词");
        item->setTextAlignment(Qt::AlignCenter);
        ui->lyrics->addItem(item);
    }
}

//根据播放器的当前播放时间与歌词里面的时间戳作对比，
//计算当前应该显示的歌词行，即当前行，
//然后根据当前行作合理的显示（高亮、滚动居中）
//逻辑：
//1.判断是否有歌词内容
//2.读取播放器的当前播放位置
//3.将当前播放时间和当前显示行的时间戳作比较：
//3.1如果播放时间小于当前行的时间位置：
//  往前找匹配的歌词行，
//  找到播放时间大于或等于某一行的时间位置，
//  即这一行就是当前应该显示的行，记录这个行的索引，即行数
//3.2如果播放时间大于当前行的时间位置：
//  往后找匹配的歌词行，
//  找到播放时间小于某一行，即它的上一行就是对应播放时间的歌词行
//3.3 如果等于
//  就是这行，啥也不干
//4.将找到的当前行高亮显示
//5.将找到的当前行滚动居中显示

void Widget:: updateLyricsOnTime()
{
   if(!m_lyrics.empty())
   {
       //如果界面还没有开始显示，就将第一行设置为当前行
       if(ui->lyrics->currentRow() == -1)//获取所在行位置
       {
           ui->lyrics->setCurrentRow(0);
           return;
       }else {//获取所在行位置
           int currentRow = ui->lyrics->currentRow();
           //读取播放器的当前播放时间位置
           qint64 playPosition = m_player->position();
           //遍历歌词容器m_lyrics,用存储了所有歌词时间部分的 keys（）做时间比较
           QList<qint64> lyricsPositions = m_lyrics.keys();
           //歌词当前行时间标签
           //qint64 currentRowPosition = lyricsPositions[currentRow];
           //比较播放器当前位置时间和歌词当前行时间
           if(playPosition < lyricsPositions[currentRow])
           {
               //当前行已经不符合，向前找 --currentRow

               while(currentRow > 0)
               {
                   //后退一行
                   --currentRow;
                   //比较播放器当前位置时间和歌词当前行时间
                   if(playPosition > lyricsPositions[currentRow])
                   {
                       break;
                   }

               }
           }else if(playPosition > lyricsPositions[currentRow])
                    {
                        //要显示的行在当前行以及后续行之中，
                        //往后找到播放时间小于某一行的时间位置，取该行的前一行
                        while(currentRow < lyricsPositions.size()-1)
                        {
                            //前进一行
                            ++currentRow;
                            //如果播放位置小于这一行，
                            //说明播放位置处于上一行的时间范围内，
                            //就后退一行才符合播放时间
                            if(playPosition < lyricsPositions[currentRow])
                            {
                                --currentRow;
                                break;
                            }
                        }
                    }
           QListWidgetItem *item = ui->lyrics->item(currentRow);

           ui->lyrics->setCurrentItem(item);
           ui->lyrics->scrollToItem(item,QAbstractItemView::PositionAtCenter);
           return;
       }
   }
}

//调用数据库管理类的打开数据库方法
bool Widget::initDatabase()
{

    bool ret = m_database->getInstance()->init();

    //创建成功返回真true
    return ret;
}

//调用数据库管理类的查询歌曲方法，查询所有的歌曲
//1）添加到成员播放列表
//2）添加到媒体播放列表
//3）添加到界面显示列表
void Widget::getPlaylistFromSql()
{
    QList<Song*> songsResult;//list 容器存储song*型内容
    bool ret = m_database->getInstance()->querySongs(songsResult);
    if(ret)
    {
        for (int i = 0;i < songsResult.size(); ++i)
        {
            m_currentPlaylist.insert(songsResult[i]->url().toString(), songsResult[i]);

            QListWidgetItem *item = new QListWidgetItem();
            item->setText(songsResult[i]->name() + "-" + songsResult[i]->artist());
            ui->playlist->addItem(item);

            m_playlist->addMedia(songsResult[i]->url());
        }
    }
    else
    {
        qDebug()<< "getPlaylistFromSql" ;
    }
}

//调用数据库管理类的添加歌曲方法，将歌曲保存到数据库
//参数：Song是要保存的歌曲对象
void Widget::addSongToSqlPlaylist(const Song& song)
{
    bool ret = m_database->getInstance()->addSong(song);
    if(ret)
    {//添加成功

    }else{
        //添加失败
        qDebug()<< "addSongToSqlPlaylist";
    }

}

//参数：
//1）filePath：mp3文件路径
//描述：
//1）打开mp3文件，跳转到最后128字节
//2）读取最后128字节
//3）将[3, 33)范围里的字节解析成歌曲名
//4）将[33, 63)范围里的字节解析成歌手名
//5) 将[63, 93)范围里的字节解析成专辑名
//6) 保存到歌曲信息类对象
void Widget::getSongInfoFromMp3File(const QString& filePath)
{
    QFile file(filePath);
    QString name, artist, album;
    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        int length = file.size();
        file.seek(length - 128);
        char infoArray[128];
        if(file.read(infoArray, sizeof(infoArray)) > 0)
        {
            QString tag = QString::fromLocal8Bit(infoArray, 3);

            name = QString::fromLocal8Bit(infoArray + 3, 30);
            artist = QString::fromLocal8Bit(infoArray + 33, 30);
            album = QString::fromLocal8Bit(infoArray + 63, 30);

            name.truncate(name.indexOf(QChar::Null));
            artist.truncate(artist.indexOf(QChar::Null));
            album.truncate(album.indexOf(QChar::Null));


        }else{

            qDebug()<<"get song info failed";
        }

        file.close();

    }else{

        qDebug()<<"open file failed";
    }

    song = new Song(filePath, name, artist, album);
    addSongToSqlPlaylist(*song);
    m_currentPlaylist.insert(filePath, song);
}

//调用数据库管理类的删除歌曲办法
void Widget::clearSqlPlaylist()
{

    m_database->getInstance()->clearSongs();
}

//数据库释放
void Widget::destroyDatabase()
{
    m_database->getInstance()->destroy();

}
//判断是否与数据库里歌曲重复 查重
bool Widget::JudgeSongRepetition(const QString& filePath)
{
    QList<Song*> songsResult;
    bool ret = m_database->getInstance()->querySongs(songsResult);
    if(ret)
    {
        for (int i = 0;i < songsResult.size();++i)
        {
            if(QUrl(filePath) == songsResult[i]->url())
            {

                return false;
            }

        }
        return true;
    }
    else
    {
        qDebug()<< "Judge Song Repetition error";
    }
}

//添加歌曲
//函数名：
//参数：
//返回值：
//描述：
//1.打开文件对话框，第三个参数指定打开的目录
//2.如果返回文件集合，遍历文件集合，对每个文件：
//2.1 设置到后台播放列表
//2.2 设置到界面列表
//2.2.1 构造一个列表元素，将文本设置到该元素上
//2.2.2 将该元素添加到界面列表
void Widget::on_add_clicked()
{
    QStringList files = QFileDialog::getOpenFileNames(this,
                "添加音乐",
                 "F:/cQT/project/music/songs",
                 "音频文件 (*.mp3)");
        bool ret;
        for (auto file : files)
        {
            ret = JudgeSongRepetition(file);
            if(ret)
            {
                QMessageBox::warning(this,"添加歌曲","添加歌曲成功");
                m_playlist->addMedia(QMediaContent(file));
                QListWidgetItem *item = new QListWidgetItem();
                item->setText(QFileInfo(QDir(file).dirName()).baseName());
                ui->playlist->addItem(item);
                getSongInfoFromMp3File(file);

            }
            else
            {
                QMessageBox::warning(this,"添加歌曲","添加歌曲失败");
            }

        }

    /*if(!files.isEmpty())
    {
        m_player->setMedia(QMediaContent(files));
        ui->songName->setText(QDir(files).dirName());
    }*/
}

//播放  暂停
//实现按钮  判断当前状态 确认启用那个函数
void Widget::on_play_clicked()
{
    if(m_player->state() != m_player->PlayingState)
    {
        m_player->play();
    }else{
        m_player->pause();
    }

}

//停止
void Widget::on_stop_clicked()
{
    m_player->stop();
}

//修改进度条，同步歌曲进度
//  调用播放器的设置进度函数，设置播放进度  99/99 0~98/99 = 0
//  计算公式：进度条的值 / 进度条的最大值 * 歌曲时长

void Widget::on_progress_sliderReleased()
{
    m_player->setPosition(double(ui->progress->value())
                          / ui->progress->maximum()
                          * m_player->duration());
}

//参数：
//1.position：位置，是播放器当前播放进度的时间值，是个毫秒数
//描述：
//1.计算公式：
//(1)进度条的值 = 当前播放位置 / 歌曲时长 * 进度条的最大值
//(2)时间标签[当前分钟数:当前秒数/时长分钟数:时长秒数]
//      当前分钟数 = 播放位置（毫秒数） / 60000;
//      当前秒数 = 播放位置（毫秒数）% 60000 / 1000;
//      时长分钟数 = 时长（毫秒数）/ 60000;
//      时长秒数 = 时长（毫秒数）% 60000 / 1000;
//逻辑
//1.进度条
//1.1获取歌曲时长
//1.2计算进度条的值
//1.3设置界面的进度条的位置
//
//2.时间标签
//1.1获取歌曲时长
//1.2计算时间标签文本中的各个值
//1.3设置界面的时间标签文本
//1.3.1 将各个时间的整数值转换成字符串
//1.3.2 将字符串设置到界面标签
void Widget::handlPlayerPositionChanged(qint64 position)
{
    qint64 duration = m_player->duration();//获取歌曲时长

    double progressValue = (double)position / duration * ui->progress->maximum();
    ui->progress->setValue(progressValue);

    char time[16] = {0};
    snprintf(time,
             sizeof(time),
             "%02lld:%02lld/%02lld:%02lld",
             position / 60000,
             position % 60000 / 1000,
             duration / 60000,
             duration % 60000 / 1000);
    ui->progressLabel->setText(time);

}

//媒体音量
void Widget::on_volume_sliderReleased()
{

    m_player->setVolume(ui->volume->value());
    QString time = QString::number(m_player->volume());
    ui->volumelabel->setText(time);

}

//
//参数：当前播放媒体，其中包含文件路径
//描述：通过文件路径获取文件名展示到界面
//2 切歌时刷新歌词
//2.1 同过音乐文件路径得出歌词文件路径
//2.2 在读取和解析歌词，储存歌词文件，显示界面
void Widget::handlePlaylistCurrentMediaChanged(const QMediaContent& content)
{
    //用参数中的歌曲文件路径与m_currentPlaylist中路径进行比较，找出对应的song
    auto item = m_currentPlaylist.begin();
    while (item != m_currentPlaylist.end())
    {
        if(QUrl(item.key()).path() == QUrl(content.canonicalUrl().toString()).path())
        {
            ui->songName->setText(item.value()->name()
                                  + "-" + item.value()->artist());
            break;
        }
        ++item;
    }

    QString mediaPath =  content.canonicalUrl().path();

        //将歌曲路径的后缀".mp3"替换成".lrc"赋值给歌词文件路径
    QString lyricsFilePath ="F:"+ mediaPath.replace(".mp3", ".lrc");

        //判断自己得出的歌词文件路径是否是一个文件
    if (QFileInfo(lyricsFilePath).isFile())
    {
            //存在歌词文件，读取歌词文件
        readLyrucsFromFile(lyricsFilePath);
            //读取完毕，显示歌词到界面
        updateLyricsAll();
    }
    else
    {
        qDebug() << "It is not a file: " << lyricsFilePath;
        updateLyricsAll();
    }
}

//播放模式 切换
void Widget::on_playbackMode_clicked()
{
    QMediaPlaylist::PlaybackMode mode = m_playlist->playbackMode();
    int nextMode = (mode + 1) % 5;
    m_playlist->setPlaybackMode(QMediaPlaylist::PlaybackMode(nextMode));
}

//播放模式图片切换
void Widget::handlePlaylistPlaybackModeChanged(QMediaPlaylist::PlaybackMode mode)
{               //modeList
    QStringList modeIconList = {":/currentItemOnce.png",//单曲
                            ":/currentItemLoop.png",//单曲循环
                            ":/sequential.png",//顺序播放
                            ":/loop.png",//循环播放
                            ":/random.png"};//随机播放

    //ui->playbackMode->setText(modeList[mode]);
    ui->playbackMode->setIcon(QPixmap(modeIconList[mode]));

}

//上一首
void Widget::on_previous_clicked()
{
    m_playlist->previous();
}

//下一首
void Widget::on_next_clicked()
{
    m_playlist->next();
}

//处理定时器超时信号的槽函数
void Widget::handleTimeout()
{
    updateLyricsOnTime();
}

//播放器播放暂停按钮图片
void Widget::handlePlayerStateChanged(QMediaPlayer::State state)
{
    if(state == QMediaPlayer::PlayingState)
    {
        ui->play->setIcon(QPixmap(":/play.png"));
    }else{
        ui->play->setIcon(QPixmap(":/pause.png"));
    }
}
//描述： 删除播放列表
//删除界面显示列表
//删除后台媒体显示列表
//删除歌曲信息
void Widget::on_clearPlaylist_clicked()
{
    m_playlist->clear();
    ui->playlist->clear();
    ui->lyrics->clear();
    ui->songName->clear();
    clearSqlPlaylist();
}

//退出
void Widget::on_quit_clicked()
{
    this->close();
}

//播放双击选中的歌曲
//获取行号
//修改索引
void Widget::on_playlist_doubleClicked(const QModelIndex &index)
{

    m_playlist->setCurrentIndex(index.row());
    m_player->play();
}
//播放双击选中的歌词
//获取行号
//确认相关行号的时间
void Widget::on_lyrics_doubleClicked(const QModelIndex &index)
{

    int i = 0;
    auto item = m_lyrics.begin();
    while (item != m_lyrics.end())
    {

        if(i == index.row())
        {
            break;
        }
        ++i;
        ++item;
    }

    m_player->setPosition(item.key());
    m_player->play();
}
//静音功能
//记录静音前音量大小
//判定当前状态进行相应操作  显示图标 设置音量
void Widget::on_toolButton_clicked()
{

    if(ui->volume->value() != 0)
    {
        VolumeTime = ui->volumelabel->text();

        m_player->setVolume(0);
        ui->volumelabel->setText("0");
        ui->volume->setValue(0);
        ui->toolButton->setIcon(QPixmap(":/ts.png"));

    }
    else
    {

        m_player->setVolume(VolumeTime.toInt());
        ui->volumelabel->setText(VolumeTime);
        ui->volume->setValue(VolumeTime.toInt());
        ui->toolButton->setIcon(QPixmap(":/th.jpg"));
    }
}
