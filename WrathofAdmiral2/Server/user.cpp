#include "user.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTimeZone>
#include "resord.h"

#ifdef max
#undef max
#endif

ResOrd User::getCurrentResources(int uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT Oil,Explo,Steel,Rub,Al,W,Cr"
                  " FROM Users WHERE UserID = :id");
    query.bindValue(":id", uid);
    query.exec();
    query.isSelect();
    if(Q_UNLIKELY(!query.first())) {
        qWarning() << qtTrId("user-nonexistent-uid").arg(uid);
        return ResOrd(ResTuple());
    }
    else {
        using namespace KP;
        ResTuple current;
        current[Oil] = query.value(0).toInt();
        current[Explosives] = query.value(1).toInt();
        current[Steel] = query.value(2).toInt();
        current[Rubber] = query.value(3).toInt();
        current[Aluminium] = query.value(4).toInt();
        current[Tungsten] = query.value(5).toInt();
        current[Chromium] = query.value(6).toInt();
        return ResOrd(current);
    }
}

const QString User::getName(int uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT Username FROM Users "
                  "WHERE UserID = :id");
    query.bindValue(":id", uid);
    query.exec();
    query.isSelect();
    if(Q_UNLIKELY(!query.first())) {
        //% "User ID %1 does not exist."
        qWarning() << qtTrId("user-nonexistent-uid").arg(uid);
        return QString();
    }
    else {
        return query.value(0).toString();
    }
}

QDateTime User::getThrottleTime(int uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT ThrottleTime FROM Users "
                  "WHERE UserID = :id");
    query.bindValue(":id", uid);
    query.exec();
    query.isSelect();
    if(Q_UNLIKELY(!query.first())) {
        //qWarning() << qtTrId("user-nonexistent-uid").arg(uid);
        return QDateTime();
    }
    else {
        return query.value(0).toDateTime();
    }
}

void User::incrementThrottleCount(int uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT ThrottleCount FROM Users "
                  "WHERE UserID = :id");
    query.bindValue(":id", uid);
    query.exec();
    query.isSelect();
    if(Q_UNLIKELY(!query.first())) {
        //qWarning() << qtTrId("user-nonexistent-uid").arg(uid);
    }
    else {
        int count = query.value(0).toInt();
        if(count < 62) {
            query.prepare("UPDATE Users SET ThrottleCount = :count "
                          "WHERE UserID = :id");
            query.bindValue(":id", uid);
            query.bindValue(":count", count+1);
            query.exec();
        }
    }
}

void User::naturalRegen(int uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT Level,RecoverTime"
                  " FROM Users WHERE UserID = :id");
    query.bindValue(":id", uid);
    query.exec();
    query.isSelect();
    if(Q_UNLIKELY(!query.first())) {
        qWarning() << qtTrId("user-nonexistent-uid").arg(uid);
        return;
    }
    else {
        const qint64 secsinMin = 60;
        int level = query.value(0).toInt();
        QDateTime priorRecoverTime = query.value(1).toDateTime();
        priorRecoverTime.setTimeZone(QTimeZone("UTC+0"));
        qint64 regenSecs = priorRecoverTime.secsTo(
                    QDateTime::currentDateTimeUtc());
        regenSecs = std::max(Q_INT64_C(0), regenSecs); //stop timezone trap
        qint64 regenMins = regenSecs / secsinMin;
        int regenPower = 0;
        ResOrd regenAmount = ResOrd(10 + regenPower,
                                    10 + regenPower,
                                    10 + regenPower,
                                    2 + regenPower,
                                    5 + regenPower,
                                    5 + regenPower,
                                    2 + regenPower);
        regenAmount *= (qint64)regenMins;
        ResOrd regenCap = ResOrd(2500,
                                 2500,
                                 2500,
                                 1500,
                                 2000,
                                 2000,
                                 1500);
        regenCap *= (qint64)(level + 24); // 24~144
        ResOrd currentRes = getCurrentResources(uid);
        currentRes.addresources(regenAmount, regenCap);
        setResources(uid, currentRes);
        QSqlQuery query;
        query.prepare("UPDATE Users SET RecoverTime "
                      "= datetime(RecoverTime, '+"
                      + QString::number(regenMins)
                      + " minutes') "
                      "WHERE UserID = :id");
        query.bindValue(":id", uid);
        if(Q_UNLIKELY(!query.exec())) {
            //% "User ID %1: natural regeneration failed!"
            qWarning() << qtTrId("natural-regen-failed").arg(uid);
            qWarning() << query.lastError();
            return;
        }
        else {
            //% "User ID %1: natural regeneration"
            qDebug() << qtTrId("natural-regen").arg(uid);
        }
    }
}

void User::refreshPort(int uid) {
    naturalRegen(uid);
}

void User::removeThrottleCount(int uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("UPDATE Users SET ThrottleCount = 0 "
                  "WHERE UserID = :id");
    query.bindValue(":id", uid);
    query.exec();
}

void User::setResources(int uid, ResOrd goal) {
    goal.cap(ResOrd(3600000,
                    3600000,
                    3600000,
                    3600000,
                    3600000,
                    3600000,
                    3600000));
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("UPDATE Users "
                  "SET Oil = :oil, "
                  "Explo = :explo, "
                  "Steel = :steel, "
                  "Rub = :rub, "
                  "Al = :al, "
                  "W = :w, "
                  "Cr = :cr "
                  "WHERE UserID = :id");
    query.bindValue(":oil", goal.oil);
    query.bindValue(":explo", goal.explo);
    query.bindValue(":steel", goal.steel);
    query.bindValue(":rub", goal.rub);
    query.bindValue(":al", goal.al);
    query.bindValue(":w", goal.w);
    query.bindValue(":cr", goal.cr);
    query.bindValue(":id", uid);
    if(Q_UNLIKELY(!query.exec())) {
        //% "User id %1: set resources failed!"
        qWarning() << qtTrId("set-resources-failed").arg(uid);
        qWarning() << query.lastError();
    }
    else {
        //% "User id %1: set resources"
        qDebug() << qtTrId("set-resources").arg(uid);
    }
}

void User::updateThrottleTime(int uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT ThrottleCount FROM Users "
                  "WHERE UserID = :id");
    query.bindValue(":id", uid);
    query.exec();
    query.isSelect();
    int count;
    if(Q_UNLIKELY(!query.first())) {
        //qWarning() << qtTrId("user-nonexistent-uid").arg(uid);
    }
    else {
        count = query.value(0).toInt();
        query.prepare("UPDATE Users "
                      "SET ThrottleTime = datetime('now', '+"
                      + QString::number(Q_INT64_C(1) << count) +
                      " seconds') "
                      "WHERE UserID = :id");
        query.bindValue(":id", uid);
        query.exec();
    }
}
