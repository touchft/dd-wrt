rm .config
rm .config.old
for i in .config*
do 
    grep "CONFIG_X86=y" $i
    if [ $? -eq 0 ] 
	then 
	    echo COPY $i
	    cp $i .config
	    grep "CONFIG_X86_64=y" $i
	    if [ $? -eq 0 ] 
		then 
		    make oldconfig ARCH=x86_64
		else
		    make oldconfig ARCH=i386
		fi
	    cp .config $i
    fi

    grep "CONFIG_ARM=y" $i
    if [ $? -eq 0 ] 
	then 
	    echo COPY $i
	    cp $i .config
	    make oldconfig ARCH=arm
	    cp .config $i
    fi

    grep "CONFIG_ARCH_IXP4XX" $i
    if [ $? -eq 0 ] 
	then 
	    echo COPY $i
	    cp $i .config
	    sed -i 's/\# CONFIG_CPU_BIG_ENDIAN is not set/CONFIG_CPU_BIG_ENDIAN=y/g' .config	    
	    make oldconfig ARCH=arm
	    cp .config $i
    fi

    grep "CONFIG_PPC32=y" $i
    if [ $? -eq 0 ] 
	then 
	    echo COPY $i
	    cp $i .config
	    make oldconfig ARCH=powerpc
	    cp .config $i
    fi

    grep "CONFIG_MIPS=y" $i
    if [ $? -eq 0 ] 
	then 
	    echo COPY $i
	    cp $i .config
	    echo CONFIG_DIR615I=y >> .config
	    echo CONFIG_WPE72=y >> .config
	    echo CONFIG_WA901=y >> .config
	    echo CONFIG_WDR4300=y >> .config
	    echo CONFIG_WDR3500=y >> .config
	    echo CONFIG_WDR2543=y >> .config
	    echo CONFIG_WR841V8=y >> .config
	    echo CONFIG_WR841V9=y >> .config
	    echo CONFIG_NVRAM_64K=y >> .config
	    echo CONFIG_NVRAM_60K=y >> .config
	    echo CONFIG_ALFANX=y >> .config
	    echo CONFIG_AP135=y >> .config
	    echo CONFIG_DAP3310=y >> .config
	    echo CONFIG_WR1043V2=y >> .config
	    echo CONFIG_ARCHERC7=y >> .config
	    echo CONFIG_DIR859=y >> .config
	    echo CONFIG_MMS344=y >> .config
	    echo CONFIG_DIR862=y >> .config
	    echo CONFIG_ERC=y >> .config
	    echo CONFIG_DAP3662=y >> .config
	    echo CONFIG_DAP2230=y >> .config
	    echo CONFIG_DAP2330=y >> .config
	    echo CONFIG_JWAP606=y >> .config
	    echo CONFIG_UAPAC=y >> .config
	    echo CONFIG_XWLOCO=y >> .config
	    echo CONFIG_WR710=y >> .config
	    make oldconfig ARCH=mips
	    sed -i 's/\CONFIG_WR841V8=y/ /g' .config	    
	    sed -i 's/\CONFIG_WR710=y/ /g' .config	    
	    sed -i 's/\CONFIG_WR841V9=y/ /g' .config	    
	    sed -i 's/\CONFIG_WPE72=y/ /g' .config	    
	    sed -i 's/\CONFIG_JWAP606=y/ /g' .config	    
	    sed -i 's/\CONFIG_DIR615I=y/ /g' .config	    
	    sed -i 's/\CONFIG_WA901=y/ /g' .config	    
	    sed -i 's/\CONFIG_WDR4300=y/ /g' .config	    
	    sed -i 's/\CONFIG_WDR3500=y/ /g' .config	    
	    sed -i 's/\CONFIG_WDR2543=y/ /g' .config	    
	    sed -i 's/\CONFIG_NVRAM_64K=y/ /g' .config	    
	    sed -i 's/\CONFIG_NVRAM_60K=y/ /g' .config	    
	    sed -i 's/\CONFIG_ALFANX=y/ /g' .config	    
	    sed -i 's/\CONFIG_AP135=y/ /g' .config	    
	    sed -i 's/\CONFIG_DAP3310=y/ /g' .config	    
	    sed -i 's/\CONFIG_WR1043V2=y/ /g' .config	    
	    sed -i 's/\CONFIG_ARCHERC7=y/ /g' .config	    
	    sed -i 's/\CONFIG_DIR859=y/ /g' .config	    
	    sed -i 's/\CONFIG_MMS344=y/ /g' .config	    
	    sed -i 's/\CONFIG_DIR862=y/ /g' .config	    
	    sed -i 's/\CONFIG_ERC=y/ /g' .config	    
	    sed -i 's/\CONFIG_DAP3662=y/ /g' .config	    
	    sed -i 's/\CONFIG_DAP2230=y/ /g' .config	    
	    sed -i 's/\CONFIG_DAP2330=y/ /g' .config	    
	    sed -i 's/\CONFIG_UAPAC=y/ /g' .config	    
	    sed -i 's/\CONFIG_XWLOCO=y/ /g' .config	    
	    cp .config $i
    fi
done
