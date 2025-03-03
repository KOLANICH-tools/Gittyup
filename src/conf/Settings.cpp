//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Settings.h"
#include "ConfFile.h"
#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>

#include <QDebug>

#ifdef Q_OS_WIN
#define CS Qt::CaseInsensitive
#else
#define CS Qt::CaseSensitive
#endif

namespace {

const QString kIgnoreWsKey = "diff/whitespace/ignore";
const QString kLastPathKey = "lastpath";

// Look up variant at key relative to root.
QVariant lookup(const QVariantMap &root, const QString &key)
{
  QStringList list = key.split("/", QString::SkipEmptyParts);
  if (list.isEmpty())
    return root;

  QVariantMap map = root;
  while (map.contains(list.first())) {
    QVariant result = map.value(list.takeFirst());
    if (list.isEmpty())
      return result;
    map = result.toMap();
  }

  return QVariant();
}

QString promptKey(Settings::PromptKind kind)
{
  QString key;
  switch (kind) {
    case Settings::PromptMerge:
      key = "merge";
      break;

    case Settings::PromptStash:
      key = "stash";
      break;

    case Settings::PromptRevert:
      key = "revert";
      break;

    case Settings::PromptCherryPick:
      key = "cherrypick";
      break;

    case Settings::PromptDirectories:
      key = "directories";
      break;

    case Settings::PromptLargeFiles:
      key = "largeFiles";
      break;
  }

  return QString("window/prompt/%1").arg(key);
}

QDir rootDir()
{
  QDir dir(QCoreApplication::applicationDirPath());

#ifdef Q_OS_MAC
  dir.cdUp(); // Contents
#endif

  return dir;
}

} // anon. namespace

Settings::Settings(QObject *parent)
  : QObject(parent)
{
  foreach (const QFileInfo &file, confDir().entryInfoList(QStringList("*.lua")))
    mDefaults[file.baseName()] = ConfFile(file.absoluteFilePath()).parse();
  mDefaults[kLastPathKey] = QDir::homePath();
  mCurrentMap = mDefaults;
}

QString Settings::group() const
{
  return mGroup.join("/");
}

void Settings::beginGroup(const QString &prefix)
{
  mGroup.append(prefix);
  mCurrentMap = lookup(mDefaults, group()).toMap();
}

void Settings::endGroup()
{
  mGroup.removeLast();
  mCurrentMap = lookup(mDefaults, group()).toMap();
}

QVariant Settings::value(const QString &key) const
{
  QSettings settings;
  settings.beginGroup(group());
  QVariant result = settings.value(key, defaultValue(key));
  settings.endGroup();
  return result;
}

QVariant Settings::defaultValue(const QString &key) const
{
  return lookup(mCurrentMap, key);
}

void Settings::setValue(const QString &key, const QVariant &value, bool refresh)
{
  QSettings settings;
  settings.beginGroup(group());
  if (value == defaultValue(key)) {
    if (settings.contains(key)) {
      settings.remove(key);
      emit settingsChanged(refresh);
    }
  } else {
    if (value != settings.value(key)) {
      settings.setValue(key, value);
      emit settingsChanged(refresh);
    }
  }
  settings.endGroup();
}

QString Settings::lexer(const QString &filename)
{
  if (filename.isEmpty())
    return "null";

  QFileInfo info(filename);
  QString name = info.fileName();
  QString suffix = info.suffix().toLower();

  // Try all patterns first.
  QVariantMap lexers = mDefaults.value("lexers").toMap();
  foreach (const QString &key, lexers.keys()) {
    QVariantMap map = lexers.value(key).toMap();
    if (map.contains("patterns")) {
      foreach (QString pattern, map.value("patterns").toString().split(",")) {
        QRegExp regExp(pattern, CS, QRegExp::Wildcard);
        if (regExp.exactMatch(name))
          return key;
      }
    }
  }

  // Try to match by extension.
  foreach (const QString &key, lexers.keys()) {
    QVariantMap map = lexers.value(key).toMap();
    if (map.contains("extensions")) {
      foreach (QString ext, map.value("extensions").toString().split(",")) {
        if (suffix == ext)
          return key;
      }
    }
  }

  return "null";
}

QString Settings::kind(const QString &filename)
{
  QString key = lexer(filename);
  QVariantMap lexers = mDefaults.value("lexers").toMap();
  return lexers.value(key).toMap().value("name").toString();
}

bool Settings::prompt(PromptKind kind) const
{
  return value(promptKey(kind)).toBool();
}

void Settings::setPrompt(PromptKind kind, bool prompt)
{
  setValue(promptKey(kind), prompt);
}

QString Settings::promptDescription(PromptKind kind) const
{
  switch (kind) {
    case PromptStash:
      return tr("Prompt to edit stash message before stashing");

    case PromptMerge:
      return tr("Prompt to edit commit message before merging");

    case PromptRevert:
      return tr("Prompt to edit commit message before reverting");

    case PromptCherryPick:
      return tr("Prompt to edit commit message before cherry-picking");

    case PromptDirectories:
      return tr("Prompt to stage directories");

    case PromptLargeFiles:
      return tr("Prompt to stage large files");
  }
}

bool Settings::isWhitespaceIgnored() const
{
  return value(kIgnoreWsKey).toBool();
}

void Settings::setWhitespaceIgnored(bool ignored)
{
  setValue(kIgnoreWsKey, ignored, true);
}

QString Settings::lastPath() const
{
  return value(kLastPathKey).toString();
}

void Settings::setLastPath(const QString &lastPath)
{
  setValue(kLastPathKey, lastPath);
}

QDir Settings::appDir()
{
  QDir dir(QCoreApplication::applicationDirPath());

#ifdef Q_OS_MAC
  dir.cdUp(); // Contents
  dir.cdUp(); // bundle
  dir.cdUp(); // bundle dir
#endif

  return dir;
}

QDir Settings::docDir()
{
  QDir dir = rootDir();
  if (!dir.cd("doc"))
    dir = confDir();
  return dir;
}

QDir Settings::confDir()
{
  QDir dir = rootDir();
  if (!dir.cd("Resources"))
    dir = QDir(CONF_DIR);
  return dir;
}

QDir Settings::l10nDir()
{
  QDir dir = confDir();
  if (!dir.cd("l10n"))
    dir = QDir(L10N_DIR);

  qDebug() << "l10n dir: " << dir;
  return dir;
}

QDir Settings::dictionariesDir()
{
  QDir dir = confDir();
  dir.cd("dictionaries");
  qDebug() << "Dictionaries dir: " << dir;
  return dir;
}

QDir Settings::lexerDir()
{
  QDir dir = confDir();
  if (!dir.cd("lexers"))
    dir = QDir(SCINTILLUA_LEXERS_DIR);
  qDebug() << "Lexers dir: " << dir;
  return dir;
}

QDir Settings::themesDir()
{
  QDir dir = confDir();
  dir.cd("themes");
  qDebug() << "Theme dir: " << dir;
  return dir;
}

QDir Settings::pluginsDir()
{
  QDir dir = confDir();
  dir.cd("plugins");
  qDebug() << "Plugins dir: " << dir;
  return dir;
}

QDir Settings::userDir()
{
  return QStandardPaths::writableLocation(
           QStandardPaths::AppLocalDataLocation);
}

QDir Settings::tempDir()
{
  QString name = QCoreApplication::applicationName();
  QDir dir = QDir::temp();
  dir.mkpath(name);
  dir.cd(name);
  return dir;
}

Settings *Settings::instance()
{
  static Settings *instance = nullptr;
  if (!instance)
    instance = new Settings(qApp);

  return instance;
}
