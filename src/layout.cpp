#include <iostream>
#include <vector>
#include <fstream>
#include "Rack.hpp"
#include "layout.hpp"
//#include <jsoncpp/json/json.h>
#include "json.hpp"
#include "nvmeResource.hpp"
using namespace std;
//using namespace Json;
using json = nlohmann::json;

void Layout::generateLayout(string filePath) {
   ifstream i(filePath);
   json j;
   i >> j;
   int rackId = 0;
   this->racks = vector<Rack>(j.size());
   for(json::iterator it = j.begin(); it!=j.end(); ++it) {
      Rack newRack = Rack();
      json newArray = it.value();
      vector<NvmeResource> nvmes;
      for(json::iterator it2 = newArray["nvmes"].begin(); it2!=newArray["nvmes"].end(); ++it2) {
         NvmeResource newNvme(it2.value()["bandwidth"],it2.value()["capacity"]);
         nvmes.push_back(newNvme);
      }
      newRack.addNvmeResourceVector(nvmes);
      this->racks[rackId++] = newRack;
   }
}