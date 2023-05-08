# Home Cluster

## Description

This is fully autonomous and automated high availability "micro data center" for home. All critical components has been reserved both on the hardware level and on the software level.

The [1st version](old-versions/version-1) on 12V was unstable when all 6 computers were turned on. It's a long story, but in a few words from electronics "axioms" - if you want to eliminate issues with power - power your electronics with proper voltage and power. Therefore, was created [2nd version](old-versions/version-2) on 24V which was stable, but I decided to improve it and build 3rd version. This version on 24V also, but without [Low voltage UPS for smart home](components/archive/smart-low-voltage-ups) (replaced by extenal UPS), [Rack Alarm System](components/archive/rack-alarm) (replaced by external smoke alarm system) and [PJON routers](components/archive/pjon-routers) (I don't use PJON for external devices, therefore, all internal devices was switched to I2C protocol). After that little bit decreased energy consumption and become better cooling of cluster (as more free space become available).

Beside automation and reservation all devices (like computers, switches, external wifi routers, etc.) fully controlled remotely:

- turn on/off/reboot via [Power Supply boards and USB HUBs](components/power-supply-usb-hubs) module;
- access to all computers via [IP-KVM](components/ip-kvm).

### Hardware level reservation

- cluster hardware has `3 x arm SBCs` and `3 x x64 mini PCs` which totally enough for building HA Kubernetes cluster (see details: [Cluster case and hardware](components/cluster-case-and-hardware));
- 2 x [USB 3.0 KVM Switcher 2 Port PCs Sharing 4 Devices](https://www.aliexpress.com/item/4001215985508.html?spm=a2g0s.9042311.0.0.27424c4dnp0HBe) used for reservation connection of USB devices;
- availability to use multiple internet providers include mobile and satellite internet providers (rack has all necessary outside connectors which can be easily connected to internal cluster components) for full reservation of internet connection.

### Software level reservation

- HA Kubernetes cluster: 3 x arm SBCs for Master nodes and 3 x x64 mini PCs for Workers;
- Each Worker node has additional 1TB disk which used for HA data storage based on [OpenEBS](https://openebs.io) and [MinIO](https://min.io);
- Full remote access to Masters and Workers via [IP-KVM](components/ip-kvm) for example to easily access in BIOS or remote manual OS installation.

### Automation and monitoring

As electronics are just my hobby and my primary job/position is SRE I clearly understand that repeatable things should be automated and critical components should be monitored.  

#### Automation

At first sight, what can be repeatable for a home cluster where hardware for the years can be unchangeable? Yes, with one server - it's can be ok, but when you have 6 servers and time to time they should be upgraded both on the hardware and on the software levels - manual deployment and configuration become to pain, therefore, I trying to keep everything automated.

- OS deployment on all nodes (includes arm SBCs) I making via PXE and process fully automated. [IP-KVM](components/ip-kvm) is used only when need make some correction in configuration files for the new OS version of automated deployment;
- for automate configuration of hardware nodes, LXD containers and applications like [HashiCorp Vault](https://www.vaultproject.io) I use [Pulumi](https://www.pulumi.com) and [Ansible](https://www.ansible.com);
- Kubernetes deployment, include building docker containers via [Bazel GitOps Rules](https://github.com/adobe/rules_gitops).

Some other software which I use:

- [Kube-vip](https://kube-vip.io) - network load balancer for bare-metal clusters;
- [Traefik](https://doc.traefik.io/traefik) - as Kubernetes ingress controller;
- [Docker-registry](https://docs.docker.com/registry) - for storing my containers;
- [OpenVPN](https://openvpn.net) - access into private network from anywhere (I have everything closed for public internet);
- [Gitea](https://gitea.io) - for storing my projects code and docs in git, also, for CI-CD, tickets system (sometimes it useful create tickets for my self :) );
- [Home Assistant](https://www.home-assistant.io) - main component for IoT things;
- [Frigate](https://github.com/blakeblackshear/frigate) - NVR With Realtime Object Detection for IP Cameras.

#### Monitoring

For hardware level, almost each line has a monitoring of voltage, current and power consumption. Also, it has 10 temperature sensors. From all these components data sending to cluster via I2C protocol and storing it in the DB for visualization via [Prometheus](https://prometheus.io) / [Grafana](https://grafana.com) with alerting about abhormal situations via [Prometheus Alertmanager](https://prometheus.io/docs/alerting/latest/alertmanager) -> [Telegram](https://telegram.org) and [Slack](https://slack.com).

For software level monitoring and alerting use the same software stack: [Prometheus](https://prometheus.io), [Grafana](https://grafana.com), [Prometheus Alertmanager](https://prometheus.io/docs/alerting/latest/alertmanager) -> [Telegram](https://telegram.org) and [Slack](https://slack.com).

How overview cluster dashboard looks:

![Cluster Dashboard](images/overview_dashboard.png)

## Cluster rack design

If you interested, check [version #1](old-versions/version-1/README.md#cluster-and-power-supply-rack) and [version #2](old-versions/version-2/README.md#cluster-rack-design).

For cluster rack I chose [TUFFIOM 9U Network Cabinet Enclosure](https://www.amazon.com/gp/product/B079ZZ8Y6X/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1) and added connectors to it to avoid pulling wires from outside through holes, i.e. isolated it from outside. This rack comes with 2 x 110V fans which I replaced by 2 x 120mm 12V fans. Also was added 2 x 120mm 12V to each rack side.

Also I use [12U rack](https://www.amazon.com/dp/B07JB9YCJF?psc=1&ref=ppx_yo2ov_dt_b_product_details) for UPS ([APC BE600M1](https://www.amazon.com/dp/B01FWAZEIU?psc=1&ref=ppx_yo2ov_dt_b_product_details)) and PoE Switch ([NETGEAR GS305EP](https://www.amazon.com/dp/B08LR18SC4?psc=1&ref=ppx_yo2ov_dt_b_product_details)) which I need for my cameras. To be sure that top 9U rack won't overweight native 12U legs I added [4 x adjustable legs](https://www.amazon.com/dp/B0B9J482TM?psc=1&ref=ppx_yo2ov_dt_b_product_details).

Holes for mounting closed by [3M Fire Barrier](https://www.amazon.com/gp/product/B002FYAMPM/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1).

I tried to make it a safe as possible:

- fully isolate everything from outside (via nonflammable connectors)
- I think that I used more 70% nonflammable components and wires inside
- I used here so many fuses as I have never seen on any device before (each line at least has one fuse, in some cases two)

But for "better sleep" I decided to put **3** automatic fire suppressors inside cluster 9U rack: 2 x [StoveTop FireStop Rangehood](https://stovetopfirestop.com/product/rangehood/) (near the largest congestion of wires) and 1 x [JOSEOZSTA](https://www.amazon.com/dp/B09YS6DJWN?ref=ppx_yo2ov_dt_b_product_details&th=1) on the top (between fans). Inside 12U rack I put 1 x [Automatic Fire Extinguisher](https://www.amazon.com/dp/B095S3L4GT?psc=1&ref=ppx_yo2ov_dt_b_product_details). All external wires was placed in [fiberglass tube](https://www.aliexpress.us/item/2255800366654895.html?spm=a2g0o.order_list.0.0.21ef1802x8n4oh&gatewayAdapt=glo2usa&_randl_shipto=US) which is totally non flammable.

Also, was added 2 external smoke detectors: 1 x [X-Sense SC06-W Smoke and Carbon Monoxide Detector](https://www.amazon.com/dp/B09FXXXG95?psc=1&ref=ppx_yo2ov_dt_b_product_details) which paired with other smoke detectors (if one will be triggered - all will be activated) and 1 x [First Alert Z-Wave Smoke Detector & Carbon Monoxide Alarm](https://www.amazon.com/dp/B08FFB233Y?psc=1&ref=ppx_yo2ov_dt_b_product_details) which I use for notifications on mobile.

[<img src="images/home_cluster_1.jpeg" width="200"/>](images/home_cluster_1.jpeg)
[<img src="images/home_cluster_2.jpeg" width="200"/>](images/home_cluster_2.jpeg)
[<img src="images/home_cluster_3.jpeg" width="200"/>](images/home_cluster_3.jpeg)
[<img src="images/home_cluster_4.jpeg" width="200"/>](images/home_cluster_4.jpeg)
[<img src="images/home_cluster_5.jpeg" width="200"/>](images/home_cluster_5.jpeg)
[<img src="images/home_cluster_6.jpeg" width="200"/>](images/home_cluster_6.jpeg)
[<img src="images/home_cluster_7.jpeg" width="200"/>](images/home_cluster_7.jpeg)
[<img src="images/home_cluster_8.jpeg" width="200"/>](images/home_cluster_8.jpeg)
[<img src="images/home_cluster_9.jpeg" width="200"/>](images/home_cluster_9.jpeg)
[<img src="images/home_cluster_10.jpeg" width="350"/>](images/home_cluster_10.jpeg)
[<img src="images/home_cluster_11.jpeg" width="350"/>](images/home_cluster_11.jpeg)
[<img src="images/home_cluster_12.jpeg" width="350"/>](images/home_cluster_12.jpeg)

On the back side of rack was placed [Power Supply with Monitoring](components/ps-with-monitoring) with 5 x 50mm 12V fans and [Rack Cooling](components/cooling) module.

[<img src="images/home_cluster_ps_1.jpeg" width="350"/>](images/home_cluster_ps_1.jpeg)
[<img src="images/home_cluster_ps_2.jpeg" width="350"/>](images/home_cluster_ps_2.jpeg)
[<img src="images/home_cluster_ps_3.jpeg" width="350"/>](images/home_cluster_ps_3.jpeg)
[<img src="images/home_cluster_ps_4.jpeg" width="350"/>](images/home_cluster_ps_4.jpeg)
[<img src="images/home_cluster_ps_5.jpeg" width="350"/>](images/home_cluster_ps_5.jpeg)

## Heatmap

[<img src="images/home_cluster_heatmap_1.jpeg" width="200"/>](images/home_cluster_heatmap_1.jpeg)
[<img src="images/home_cluster_heatmap_2.jpeg" width="200"/>](images/home_cluster_heatmap_2.jpeg)
[<img src="images/home_cluster_heatmap_3.jpeg" width="350"/>](images/home_cluster_heatmap_3.jpeg)
[<img src="images/home_cluster_heatmap_4.jpeg" width="350"/>](images/home_cluster_heatmap_4.jpeg)

As it too much electronics inside 9U rack it should be very good cooled, therefore, inside this rack I placed 20 fans (11 for rack cooling and 9 for cluster cooling). Fans turn on only when temperatures higher than normal and controlled by [cooling](components/cooling) modules.
