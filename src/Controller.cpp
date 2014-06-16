/*
 *  Controller.cpp - Controller class for livemediastreamer framework
 *  Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of liveMediaStreamer.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Authors:  Marc Palau <marc.palau@i2cat.net>,
 */

#include "Controller.hh"
//#include "Callbacks.hh"

Controller* Controller::ctrlInstance = NULL;
PipelineManager* PipelineManager::pipeMngrInstance = NULL;
WorkerManager* WorkerManager::workMngrInstance = NULL;

Controller::Controller()
{    
    ctrlInstance = this;
    pipeMngrInstance = PipelineManager::getInstance();
    workMngrInstance = WorkerManager::getInstance();
}

Controller* Controller::getInstance()
{
    if (ctrlInstance != NULL){
        return ctrlInstance;
    }
    
    return new Controller();
}

void Controller::destroyInstance()
{
    Controller * ctrlInstance = Controller::getInstance();
    if (ctrlInstance != NULL) {
        delete ctrlInstance;
        ctrlInstance = NULL;
    }
}

PipelineManager* Controller::pipelineManager()
{
    return pipeMngrInstance;
}

WorkerManager* Controller::workerManager()
{
    return workMngrInstance;
}

///////////////////////////////////
//PIPELINE MANAGER IMPLEMENTATION//
///////////////////////////////////

PipelineManager::PipelineManager()
{
    pipeMngrInstance = this;
    receiverID = rand();
    transmitterID = rand();
    addFilter(receiverID, SourceManager::getInstance());
    addFilter(transmitterID, SinkManager::getInstance());
    //receiver->setCallback(callbacks::connectToMixerCallback);
}

PipelineManager* PipelineManager::getInstance()
{
    if (pipeMngrInstance != NULL) {
        return pipeMngrInstance;
    }

    return new PipelineManager();
}

void PipelineManager::destroyInstance()
{
    PipelineManager* pipeMngrInstance = PipelineManager::getInstance();
    if (pipeMngrInstance != NULL) {
        delete pipeMngrInstance;
        pipeMngrInstance = NULL;
    }
}

int PipelineManager::searchFilterIDByType(FilterType type)
{
    for (auto it : filters) {
        if (it.second.first->getType() == type) {
            return it.first;
        }
    }

    return -1;
}

bool PipelineManager::addPath(int id, Path* path)
{
    if (paths.count(id) > 0) {
        return false;
    }

    paths[id] = path;

    return true;
}

bool PipelineManager::addFilter(int id, BaseFilter* filter)
{
    if (filters.count(id) > 0) {
        return false;
    }

    filters[id] = std::pair<BaseFilter*, Worker*>(filter, NULL);

    return true;
}

bool PipelineManager::addWorker(int id, Worker* worker)
{
    if (filters.count(id) <= 0) {
        return false;
    }

    worker->setProcessor(filters[id].first);
    filters[id].second = worker;

    return true;
}

Path* PipelineManager::getPath(int id)
{
    if (paths.count(id) <= 0) {
        return NULL;
    }

    return paths[id];
}

BaseFilter* PipelineManager::getFilter(int id)
{
    if (filters.count(id) <= 0) {
        return NULL;
    }

    return filters[id].first;
}


bool PipelineManager::connectPath(Path* path)
{
    int orgFilterId = path->getOriginFilterID();
    int dstFilterId = path->getDestinationFilterID();
    
    if (filters.count(orgFilterId) <= 0) {
        return false;
    }

    if (filters.count(dstFilterId) <= 0) {
        return false;
    }

    std::vector<int> pathFilters = path->getFilters();

    if (pathFilters.empty()) {
        if (filters[orgFilterId].first->connectManyToMany(filters[dstFilterId].first, path->getDstReaderID(), path->getOrgWriterID())) {
            return true;
        } else {
            std::cerr << "Error connecting head to tail!" << std::endl;
            return false;
        }
    }

    if (!filters[orgFilterId].first->connectManyToOne(filters[pathFilters.front()].first, path->getOrgWriterID())) {
        std::cerr << "Error connecting path head to first filter!" << std::endl;
        return false;
    }

    for (unsigned i = 0; i < pathFilters.size() - 1; i++) {
        if (!filters[pathFilters[i]].first->connectOneToOne(filters[pathFilters[i+1]].first)) {
            std::cerr << "Error connecting path filters!" << std::endl;
            return false;
        }
    }

    if (!filters[pathFilters.back()].first->connectOneToMany(filters[dstFilterId].first, path->getDstReaderID())) {
        std::cerr << "Error connecting path last filter to path tail!" << std::endl;
        return false;
    }

    return true;

}

bool PipelineManager::addWorkerToPath(Path *path, Worker* worker)
{
    std::vector<int> pathFilters = path->getFilters();

    if (pathFilters.empty()) {
        //TODO: error msg
        return false;
    }

    for (auto it : pathFilters) {
        worker->setProcessor(filters[it].first);
        filters[it].second = worker;
    }
}

SourceManager* PipelineManager::getReceiver()
{
    return dynamic_cast<SourceManager*>(filters[receiverID].first);
}


SinkManager* PipelineManager::getTransmitter() 
{
    return dynamic_cast<SinkManager*>(filters[transmitterID].first);
}


/////////////////////////////////
//WORKER MANAGER IMPLEMENTATION//
/////////////////////////////////

WorkerManager::WorkerManager()
{
    workMngrInstance = this;
}

WorkerManager* WorkerManager::getInstance()
{
    if (workMngrInstance != NULL) {
        return workMngrInstance;
    }

    return new WorkerManager();
}

void WorkerManager::destroyInstance()
{
    WorkerManager * workMngrInstance = WorkerManager::getInstance();
    if (workMngrInstance != NULL) {
        delete workMngrInstance;
        workMngrInstance = NULL;
    }
}
