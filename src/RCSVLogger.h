/*
 * Copyright (C) 2019 EPFL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */

/**
 * @file RCSVLogger.h
 * @brief Header for a QML CSV file logger for online databases
 * @author Alexandre Reynaud
 * @date 2019-03-14
 */

#ifndef RCSVLOGGER_H
#define RCSVLOGGER_H

#include <QQuickItem>
#include <QString>
#include <QFile>
#include <QVariant>
#include <QtCore>
#include <QNetworkAccessManager>
#include <QMap>
#include <QStringList>
#include <QtCore>
#include <QObject>
#include <QIODevice>
#include <QByteArray>
#include <QMetaObject>
#include <QtNetwork>
#include <QUrl>
namespace QMLLogger {

/**
 * @brief Utility to log CSV data line by line with optional timestamp and store the whole thing in a remote server.
 *
 * Unless given a full path, this will dump all log actions to the file with the given name under the Documents
 * directory of the current user, whatever this is configured as under the specific OS, except Windows.
 *
 * On Windows, the log file will be put under the local data directory of the app since access is not given
 * to write to any other directory than this within WinRT sandboxing. This will typically be
 * C:\Users\your-username\AppData\Local\Packages\app-uuid\LocalState. Be careful when using the Run button
 * in QtCreator, as this will destroy this directory each time the app is launched, taking your potentially
 * valuable data with it! Instead, launch your app from the Start Menu, which will preserve this directory.
 *
 * At the first call to the `log(list<string> data)` slot, if the log file is empty
 * or newly created, a header will be dumped to the file as follows:
 *
 * ```
 *     timestamp if enabled, header[0], header[1], ..., header[N - 1]
 * ```
 *
 * After that, every call to the `log(list<string> data)` slot will result in a line as follows in the log file:
 *
 * ```
 *     timestamp in yyyy-MM-dd HH:mm:ss.zzz format if enabled, data[0], data[1], ..., data[N - 1]
 * ```
 */
    class RCSVLogger : public QQuickItem {
        /* *INDENT-OFF* */
        Q_OBJECT
        /* *INDENT-ON* */

        /** @brief Desired log filename; if full path is not given, log file will be put in default documents directory */
        Q_PROPERTY(QString filename WRITE setFilename READ getFilename NOTIFY filenameChanged)

        /** @brief Whether to include the timestamp in every log line as the first field, cannot be changed after a call to `log()` until a call to `close()`, default `true` */
        Q_PROPERTY(bool logTime WRITE setLogTime READ getLogTime NOTIFY logTimeChanged)

        /** @brief Whether to include milliseconds in date and time, default `true` */
        Q_PROPERTY(bool logMillis MEMBER logMillis)

        /** @brief Whether to print the log lines to the console for debug purposes, default `false` */
        Q_PROPERTY(bool toConsole MEMBER toConsole)

        /** @brief Number of decimal places for printing floating point numbers, default `2`*/
        Q_PROPERTY(int precision MEMBER precision)

        /** @brief Header fields (excluding timestamp), cannot be changed after a call to `log()` until a call to `close()`, default `[]` */
        Q_PROPERTY(QList<QString> header WRITE setHeader READ getHeader NOTIFY headerChanged)

        /** @brief IP of the database server */
        Q_PROPERTY(QString serverURL MEMBER serverURL)
    public:
        /** @cond DO_NOT_DOCUMENT */

        /**
         * @brief Creates a new RCSVLogger with the given QML parent
         *
         * @param parent The QML parent
         */
        RCSVLogger(QQuickItem* parent = nullptr);

        /**
         * @brief Destroys this RCSVLogger
         */
        ~RCSVLogger();

        /**
         * @brief Sets the file name; puts file to home directory if full path is not given
         *
         * @param filename The new filename, or full path
         */
        void setFilename(const QString& filename);

        /**
         * @brief Gets the filename
         *
         * @return The filename
         */
        QString getFilename(){ return filename; }

        /**
         * @brief Sets whether to log the timestamp as the first field, has no effect after the first log()
         *
         * @param logTime Whether to log the timestamp as the first field
         */
        void setLogTime(bool logTime);

        /**
         * @brief Gets whether the timestamp is being logged
         *
         * @return Whether the timestamp is being logged
         */
        bool getLogTime(){ return logTime; }

        /**
         * @brief Sets the new header to be dumped to the log file on its first open if it's empty
         *
         * @param header New header
         */
        void setHeader(QList<QString> const& header);

        /**
         * @brief Gets the current header
         *
         * @return The current header
         */
        QList<QString> getHeader(){ return header; }

        /** @endcond */

    signals:

        /** @cond DO_NOT_DOCUMENT */

        /**
         * @brief Emitted when filename changes
         */
        void filenameChanged();

        /**
         * @brief Emitted when logTime changes
         */
        void logTimeChanged();

        /**
         * @brief Emitted when the header changes
         */
        void headerChanged();
        /** @endcond */

    public slots:

        /**
         * @brief Logs given data as one entry
         *
         * @param data Data to log, must conform to the header format if meaningful log is desired
         */
        void log(QVariantList const& data);

    private:

        QString filename;              ///< Log's filename or full path
        QList<QString> header;         ///< Header to dump on the first line

        bool writing;                  ///< Log is being written

        QFile file;                    ///< Log file
        QTextStream writer;            ///< Log file writer

        bool logTime;                  ///< Whether to include timestamp as the first field when data is logged
        bool logMillis;                ///< Whether to include milliseconds in the timestamp
        bool toConsole;                ///< Log to console instead of file for debug purposes
        int precision;                 ///< Number of decimal places to print to the log for floats

        const QString timestampHeader; ///< Timestamp header field string

        const QString logManagerPath="logManager.csv"; ///< Path to the log manager

        QString serverURL; ///< IP of the database server
        QMap<QString,QList<int>> logManager; ///< Map linking each file to the number of lines they contain localy and remotely
        QMap<QString,QList<QVariantList>> updates; ///< Map linking each file to the lines yet to be written localy

        QNetworkAccessManager *manager; ///< Network manager

        /**
         * @brief Builds and gets the header string
         *
         * @return Header string, including the timestamp as the first field if logTime is true
         */
        QString buildHeaderString();

        /**
         * @brief Builds and gets the log row
         *
         * @param data Data to log
         * @return Log row
         */
        QString buildLogLine(QVariantList const& data);

        /**
         * @brief Updates the local logs
         */
        void updateLocal();

        /**
         * @brief Updates the remote database to match the local one
         */
        void updateRemote();

        /**
         * @brief Loads (or creates if non-existant) the log manager
         * @return The log manager
         */
        QMap<QString,QList<int>> loadLogManager();

        /**
         * @brief Saves the current state of the log manager
         */
        void saveLogManager();

        /**
         * @brief Loads all lines of a given file starting from a given row number
         * @param path Path to the file
         * @param from Number of the first read byte
         * @return List of all read rows
         */
        QStringList CSVReader(QString path, int from);

        /**
         * @brief Writes all given lines at the given index of the given file
         * @param path Path to the file
         * @param lines List of the rows to write
         * @param from Index from which the given lines must be written (at the end of the file by default)
         * @param log True if the lines to write are logs (false by default)
         * @return Amount of bytes written
         */
        int CSVLogWriter(QString path, QList<QVariantList> lines);

        /**
         * @brief Writes all given lines at the given index of the given file
         * @param path Path to the file
         * @param lines List of the rows to write
         * @param from Index from which the given lines must be written (at the end of the file by default)
         * @param log True if the lines to write are logs (false by default)
         * @return Amount of bytes written
         */
        int CSVManagementWriter(QString path, QList<QVariantList> lines);

        /**
         * @brief Transforms the given path into an absolute path if it isn't already
         * @param path Path to the file
         * @return Absolute path to the file
         */
        QString absolutePath(QString path);
    };

}
#endif // RCSVLOGGER_H