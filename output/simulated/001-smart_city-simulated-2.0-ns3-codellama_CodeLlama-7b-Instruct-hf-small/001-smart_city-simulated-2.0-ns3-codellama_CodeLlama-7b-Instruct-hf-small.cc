#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/lte-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/wifi-module.h"
#include "ns3/wifi-phy-module.h"
#include "ns3/wifi-channel-equalizer-module.h"
#include "ns3/wifi-psdu-signal-energy-model.h"
#include "ns3/wifi-phy-state-helper.h"
#include "ns3/wifi-mac-header-common.h"
#include "ns3/wifi-mac-trailer.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-mac-queue-item.h"
#include "ns3/wifi-mac-queue-ac.h"
#include "ns3/wifi-mac-queue-vo.h"
#include "ns3/wifi-mac-queue-vi.h"
#include "ns3/wifi-mac-queue-be.h"
#include "ns3/wifi-mac-queue-bk.h"
#include "ns3/wifi-mac-queue-ba.h"
#include "ns3/wifi-mac-queue-hs.h"
#include "ns3/wifi-mac-queue-ndp.h"
#include "ns3/wifi-mac-queue-qos-ht.h"
#include "ns3/wifi-mac-queue-qos-vht.h"
#include "ns3/wifi-mac-queue-qos-ht-er.h"
#include "ns3/wifi-mac-queue-qos-vht-er.h"
#include "ns3/wifi-mac-queue-qos-ht-ndp.h"
#include "ns3/wifi-mac-queue-qos-vht-ndp.h"
#include "ns3/wifi-mac-queue-qos-ht-ba.h"
#include "ns3/wifi-mac-queue-qos-vht-ba.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-er.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-er.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-ndp.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-ndp.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-er.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-er.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ndp.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-ndp.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-er.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-er.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ndp.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ndp.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-er.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-er.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-ndp.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-ndp.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-er.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-er.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ndp.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-ndp.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-er.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-er.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ndp.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ndp.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-er.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-er.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-ndp.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-ndp.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-er.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-er.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ndp.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-ndp.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-er.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-er.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ndp.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ndp.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-er.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-er.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-ndp.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-ndp.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-er.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca-er.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ndp.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca-ndp.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-er.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-er.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ndp.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ndp.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-er.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca-ht-ba-er.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-ndp.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca-ht-ba-ndp.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca-ht-ba-edca.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-er.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca-ht-ba-edca-er.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ndp.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca-ht-ba-edca-ndp.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca-ht-ba-edca-vht.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-er.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca-ht-ba-edca-vht-er.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ndp.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca-ht-ba-edca-vht-ndp.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca-ht-ba-edca-vht-ba.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-er.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca-ht-ba-edca-vht-ba-er.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-ndp.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca-ht-ba-edca-vht-ba-ndp.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca-ht-ba-edca-vht-ba-edca.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-er.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca-ht-ba-edca-vht-ba-edca-er.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ndp.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca-ht-ba-edca-vht-ba-edca-ndp.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca-ht-ba-edca-vht-ba-edca-vht.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-ba-edca-ht-er.h"
#include "ns3/wifi-mac-queue-qos-vht-ba-edca-vht-ba-edca-vht-ba-edca-vht-ba-edca-ht-ba-edca-vht-ba-edca-vht-er.h"
#include "ns3/wifi-mac-queue-qos-ht-ba-edca-ht-ba-edca-ht-ba-edca