/*
 * This file is part of QtEmu project.
 * Copyright (C) 2006-2009 Urs Wolfer <uwolfer @ fwo.ch> and Ben Klopfenstein <benklop gmail com>
 * Copyright (C) 2017-2019 Sergio Carlavilla <carlavilla @ mailbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

// Local
#include "machine.h"
#include "machineutils.h"

MachineUtils::MachineUtils(QObject *parent) : QObject(parent)
{
    qDebug() << "MachineUtils object created";
}

MachineUtils::~MachineUtils()
{
    qDebug() << "MachineUtils object destroyed";
}

/**
 * @brief Read the machine file and inform the machineJSON
 *
 * @param machinePath, path of the machine config
 *
 * @return QByteArray with the data
 */
QJsonObject MachineUtils::readMachineFile(QString machinePath)
{
    QJsonObject machineJSON;

    // TODO: Move all to generic function to check if file can be opened or readed
    QFile machineFile(machinePath);
    if (!machineFile.open(QFile::ReadOnly)) {
        QMessageBox *machinePathMessageBox = new QMessageBox();
        machinePathMessageBox->setWindowTitle(tr("Qtemu - Critical error"));
        machinePathMessageBox->setIcon(QMessageBox::Critical);
        machinePathMessageBox->setWindowIcon(QIcon::fromTheme("qtemu", QIcon(":/images/qtemu.png")));
        machinePathMessageBox->setText(tr(qPrintable(QString("<p>Cannot load the machine</p>"
                                                               "<p>Cannot open the <strong>%1</strong> file. "
                                                               "Please ensure that the file exists and it's readable</p>").arg(machinePath))));
        machinePathMessageBox->exec();
        return machineJSON;
    }

    QByteArray machineData = machineFile.readAll();
    QJsonDocument machineDocument(QJsonDocument::fromJson(machineData));
    machineJSON = machineDocument.object();

    if (machineFile.isOpen()) {
        machineFile.close();
    }

    return machineJSON;
}

/**
 * @brief Fill the machine object with the data
 *
 * @param machine, machine object
 * @param machineJSON, JSON with the machine data
 * @param machineConfigPath, path of the machine
 *
 * Fill the machine object with the data
 */
void MachineUtils::fillMachineObject(Machine *machine,
                                     QJsonObject machineJSON, QString machineConfigPath)
{
    QJsonObject gpuObject = machineJSON["gpu"].toObject();
    QJsonObject cpuObject = machineJSON["cpu"].toObject();
    QJsonObject bootObject = machineJSON["boot"].toObject();
    QJsonObject kernelObject = bootObject["kernelBoot"].toObject();
    QJsonArray mediaArray = machineJSON["media"].toArray();

    Boot *machineBoot = new Boot(machine);
    machineBoot->setBootMenu(bootObject["bootMenu"].toBool());
    machineBoot->setKernelBootEnabled(kernelObject["enabled"].toBool());
    machineBoot->setKernelPath(kernelObject["kernelPath"].toString());
    machineBoot->setInitrdPath(kernelObject["initrdPath"].toString());
    machineBoot->setKernelArgs(kernelObject["kernelArgs"].toString());
    machineBoot->setBootOrder(MachineUtils::getMediaDevices(bootObject["bootOrder"].toArray()));

    for(int i = 0; i < mediaArray.size(); ++i) {
        QJsonObject mediaObject = mediaArray[i].toObject();

        Media *media = new Media(machine);
        media->setName(mediaObject["name"].toString());
        media->setPath(mediaObject["path"].toString());
        media->setType(mediaObject["type"].toString());
        media->setDriveInterface(mediaObject["interface"].toString());
        media->setUuid(mediaObject["uuid"].toVariant().toUuid());
        machine->addMedia(media);
    }

    machine->setState(Machine::Stopped);
    machine->setName(machineJSON["name"].toString());
    machine->setOSType(machineJSON["OSType"].toString());
    machine->setOSVersion(machineJSON["OSVersion"].toString());
    machine->setType(machineJSON["type"].toString());
    machine->setDescription(machineJSON["description"].toString());
    machine->setRAM(machineJSON["RAM"].toInt());
    machine->setUseNetwork(machineJSON["network"].toBool());
    machine->setConfigPath(machineConfigPath);
    machine->setPath(machineJSON["path"].toString());
    machine->setUuid(machineJSON["uuid"].toString());
    machine->setGPUType(gpuObject["GPUType"].toString());
    machine->setKeyboard(gpuObject["keyboard"].toString());
    machine->setCPUType(cpuObject["CPUType"].toString());
    machine->setCPUCount(cpuObject["CPUCount"].toInt());
    machine->setCoresSocket(cpuObject["coresSocket"].toInt());
    machine->setMaxHotCPU(cpuObject["maxHotCPU"].toInt());
    machine->setSocketCount(cpuObject["socketCount"].toInt());
    machine->setThreadsCore(cpuObject["threadsCore"].toInt());
    machine->setHostSoundSystem(machineJSON["hostsoundsystem"].toString());
    machine->setAudio(MachineUtils::getSoundCards(machineJSON["audio"].toArray()));
    machine->setAccelerator(MachineUtils::getAccelerators(machineJSON["accelerator"].toArray()));
    machine->setBoot(machineBoot);
}

/**
 * @brief Delete the machine
 * @param machineUuid, uuid of the machine
 * @return true if the machine is deleted
 *
 * Delete all the files of the machine
 */
bool MachineUtils::deleteMachine(const QUuid machineUuid)
{
    QSettings settings;
    settings.beginGroup("DataFolder");
    QString dataDirectoryPath = settings.value("QtEmuData",
                                               QDir::toNativeSeparators(QDir::homePath() + "/.qtemu/")).toString();
    settings.endGroup();

    // Open the file with the machines
    QString qtemuConfig = dataDirectoryPath.append("qtemu.json");
    QFile machinesFile(qtemuConfig);
    if (!machinesFile.open(QFile::ReadWrite)) {
        QMessageBox *m_deleteMachineMessageBox = new QMessageBox();
        m_deleteMachineMessageBox->setWindowTitle(tr("Qtemu - Critical error"));
        m_deleteMachineMessageBox->setIcon(QMessageBox::Critical);
        m_deleteMachineMessageBox->setWindowIcon(QIcon::fromTheme("qtemu", QIcon(":/images/qtemu.png")));
        m_deleteMachineMessageBox->setText(tr("<p>Cannot delete the machine</p>"
                                              "<p>The file with the machines configuration are not readable</p>"));
        m_deleteMachineMessageBox->exec();
        return false;
    }

    // Read all the machines included in the file
    QByteArray machinesData = machinesFile.readAll();
    QJsonDocument machinesDocument(QJsonDocument::fromJson(machinesData));
    QJsonArray machines = machinesDocument["machines"].toArray();

    int machinePos = 0;
    bool machineExists = false;
    QString machinePath;
    while(machinePos < machines.size() && ! machineExists) {
        QJsonObject machineJSON = machines[machinePos].toObject();
        if (machineUuid == machineJSON["uuid"].toVariant()) {
            machineExists = true;
            machinePath = machineJSON["path"].toString();
        } else {
            ++machinePos;
        }
    }

    if (machineExists) {
        machines.removeAt(machinePos);
    }

    QJsonObject machinesObject;
    machinesObject["machines"] = machines;
    QJsonDocument machinesDocumentJSON(machinesObject);

    machinesFile.seek(0);
    machinesFile.resize(0);
    machinesFile.write(machinesDocumentJSON.toJson());
    if (machinesFile.isOpen()) {
        machinesFile.close();
    }

    QDir *machineDirectory = new QDir(QDir::toNativeSeparators(machinePath));
    bool removedDirectory = machineDirectory->removeRecursively();

    return removedDirectory;
}

/**
 * @brief Get the sound cards
 * @param soundCardsArray, json array with the sound cards of the machine
 * @return list with the sound cards
 *
 * Get the sound cards
 */
QStringList MachineUtils::getSoundCards(QJsonArray soundCardsArray)
{
    QStringList soundCardsList;
    for(int i = 0; i < soundCardsArray.size(); ++i) {
        soundCardsList.append(soundCardsArray[i].toString());
    }

    return soundCardsList;
}

/**
 * @brief Get the accelerators
 * @param acceleratorsArray, json array with the accelerators of the machine
 * @return list with the accelerators
 *
 * Get the accelerators
 */
QStringList MachineUtils::getAccelerators(QJsonArray acceleratorsArray)
{
    QStringList acceleratorsList;
    for(int i = 0; i < acceleratorsArray.size(); ++i) {
        acceleratorsList.append(acceleratorsArray[i].toString());
    }

    return acceleratorsList;
}

/**
 * @brief Get the media devices
 * @param mediaDevicesArray, json array with the media devices of the machine
 * @return list with the media devices
 *
 * Get the media devices
 */
QStringList MachineUtils::getMediaDevices(QJsonArray mediaDevicesArray)
{
    QStringList mediaDevicesList;
    for(int i= 0; i < mediaDevicesArray.size(); ++i) {
        mediaDevicesList.append(mediaDevicesArray[i].toString());
    }

    return mediaDevicesList;
}
