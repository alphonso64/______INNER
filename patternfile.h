#ifndef PATTERNFILE_H
#define PATTERNFILE_H
#include <vector>
#include <QString>
#include <QMap>
#include "algorithm/pic.h"
#include <QDebug>
using namespace std;
class PatternFile
{
public:
    PatternFile();
    int th;
    int rotate;
    QString path;
    QString name;
    void selfRelease();
    vector<Pic<uchar>> temps;
};

#endif // PATTERNFILE_H
