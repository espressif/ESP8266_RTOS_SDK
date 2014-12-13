Enable USB Support
------------------

If you want to program the device directly from the Virtual Machine, you can 
add the USB Serial adapter with the following commands (this assumes that 
the virtual machine is running):


	export VEND_ID="PUT THE VENDOR ID HERE"
	export PROD_ID="PUT THE PRODUCT ID HERE"

	export VM_NAME=$( vboxmanage list runningvms | cut -d'"' -f2 )
	VBoxManage modifyvm "${VM_NAME}" --usb on --usbehci on

	VBoxManage usbfilter add 0 --target ${VM_NAME} --name usb_adapter \
               --vendorid ${VEND_ID} --productid ${PROD_ID}


You can get the VendorId and ProductId using `lsusb`
