#! /bin/bash

	ARCH=$( uname )
	if [ "$ARCH" != "Linux" ]; then
         printf "\\n\\tAuction currently support Amazon, Centos, Fedora, Mint & Ubuntu Linux only.\\n"
         printf "\\tPlease install on the latest version of one of these Linux distributions.\\n"
         printf "\\thttps://aws.amazon.com/amazon-linux-ami/\\n"
         printf "\\thttps://www.centos.org/\\n"
         printf "\\thttps://start.fedoraproject.org/\\n"
         printf "\\thttps://linuxmint.com/\\n"
         printf "\\thttps://www.ubuntu.com/\\n"
         printf "\\tExiting now.\\n"
         exit 1
	fi
	DISK_MIN="5"

	OS_VER=$( grep VERSION_ID /etc/os-release | cut -d'=' -f2 | sed 's/[^0-9\.]//gI' )
	OS_MAJ=$(echo "${OS_VER}" | cut -d'.' -f1)
	OS_MIN=$(echo "${OS_VER}" | cut -d'.' -f2)

	MEM_MEG=$( free -m | sed -n 2p | tr -s ' ' | cut -d\  -f2 || cut -d' ' -f2 )
	CPU_SPEED=$( lscpu | grep -m1 "MHz" | tr -s ' ' | cut -d\  -f3 || cut -d' ' -f3 | cut -d'.' -f1 )
	CPU_CORE=$( lscpu -pCPU | grep -v "#" | wc -l )

	MEM_GIG=$(( ((MEM_MEG / 1000) / 2) ))
	JOBS=$(( MEM_GIG > CPU_CORE ? CPU_CORE : MEM_GIG ))

	DISK_INSTALL=$(df -h . | tail -1 | tr -s ' ' | cut -d\  -f1 || cut -d' ' -f1)
	DISK_TOTAL_KB=$(df . | tail -1 | awk '{print $2}')
	DISK_AVAIL_KB=$(df . | tail -1 | awk '{print $4}')
	DISK_TOTAL=$(( DISK_TOTAL_KB / 1048576 ))
	DISK_AVAIL=$(( DISK_AVAIL_KB / 1048576 ))

	printf "\\n\\tOS name: %s\\n" "${OS_NAME}"
	printf "\\tOS Version: %s\\n" "${OS_VER}"
	printf "\\tCPU speed: %sMhz\\n" "${CPU_SPEED}"
	printf "\\tCPU cores: %s\\n" "${CPU_CORE}"
	printf "\\tPhysical Memory: %s Mgb\\n" "${MEM_MEG}"
	printf "\\tDisk install: %s\\n" "${DISK_INSTALL}"
	printf "\\tDisk space total: %sG\\n" "${DISK_TOTAL%.*}"
	printf "\\tDisk space available: %sG\\n" "${DISK_AVAIL%.*}"

	if [ "${MEM_MEG}" -lt 1900 ]; then
		printf "\\tYour system must have 2 or more Gigabytes of physical memory installed.\\n"
		printf "\\tExiting now.\\n"
		exit 1
	fi

	case "${OS_NAME}" in
		"Linux Mint")
		   if [ "${OS_MAJ}" -lt 18 ]; then
			   printf "\\tYou must be running Linux Mint 18.x or higher to install Auction.\\n"
			   printf "\\tExiting now.\\n"
			   exit 1
		   fi
		;;
		"Ubuntu")
			if [ "${OS_MAJ}" -lt 16 ]; then
				printf "\\tYou must be running Ubuntu 16.04.x or higher to install Auction.\\n"
				printf "\\tExiting now.\\n"
				exit 1
			fi
		;;
		"Debian")
			if [ $OS_MAJ -lt 10 ]; then
				printf "\tYou must be running Debian 10 to install Auction, and resolve missing dependencies from unstable (sid).\n"
				printf "\tExiting now.\n"
				exit 1
		fi
		;;
	esac

	if [ "${DISK_AVAIL%.*}" -lt "${DISK_MIN}" ]; then
		printf "\\tYou must have at least %sGB of available storage to install Auction.\\n" "${DISK_MIN}"
		printf "\\tExiting now.\\n"
		exit 1
	fi

	DEP_ARRAY=(g++ cmake make automake libssl-dev build-essential googletest \
    autoconf libtool doxygen libgoogle-perftools-dev libboost-dev)
	COUNT=1
	DISPLAY=""
	DEP=""

	printf "\\n\\tChecking for installed dependencies.\\n\\n"

	for (( i=0; i<${#DEP_ARRAY[@]}; i++ ));
	do
		pkg=$( dpkg -s "${DEP_ARRAY[$i]}" 2>/dev/null | grep Status | tr -s ' ' | cut -d\  -f4 )
		if [ -z "$pkg" ]; then
			DEP=$DEP" ${DEP_ARRAY[$i]} "
			DISPLAY="${DISPLAY}${COUNT}. ${DEP_ARRAY[$i]}\\n\\t"
			printf "\\tPackage %s ${bldred} NOT ${txtrst} found.\\n" "${DEP_ARRAY[$i]}"
			(( COUNT++ ))
		else
			printf "\\tPackage %s found.\\n" "${DEP_ARRAY[$i]}"
			continue
		fi
	done		

	if [ "${COUNT}" -gt 1 ]; then
		printf "\\n\\tThe following dependencies are required to install Auction.\\n"
		printf "\\n\\t${DISPLAY}\\n\\n" 
		printf "\\tNeed to install these packages\\n"
		printf "\\n\\n\\tInstalling dependencies\\n\\n"
		sudo apt-get update
		if ! sudo apt-get -y install ${DEP}
		then
			printf "\\n\\tDPKG dependency failed.\\n"
			printf "\\n\\tExiting now.\\n"
			exit 1
		else
			printf "\\n\\tDPKG dependencies installed successfully.\\n"
		fi
	else 
		printf "\\n\\tNo required dpkg dependencies to install.\\n"
	fi

    if [ ! -f /usr/local/lib/pkgconfig/benchmark.pc ]; then
        # to be install google benchmark
		git clone https://github.com/google/benchmark.git
		git clone https://github.com/google/googletest.git benchmark/googletest
        [ -d benchmark/build.test ] || mkdir benchmark/build.test
        (cd benchmark/build.test; \
            cmake ../googletest -DCMAKE_BUILD_TYPE=Release; \
            make -j"$JOBS"; sudo make install)
        [ -d benchmark/build ] || mkdir benchmark/build
        (cd benchmark/build; \
            cmake .. -DCMAKE_BUILD_TYPE=Release -DBENCHMARK_ENABLE_TESTING=OFF; \
            make -j"$JOBS"; sudo make install)
    fi
