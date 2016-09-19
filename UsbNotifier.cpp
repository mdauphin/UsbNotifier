#include <windows.h>
#include <setupapi.h>
#include <stdio.h>

const GUID GUID_DEVINTERFACE_HID = {0x4d1e55b2, 0xf16f, 0x11cf, {0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30} };

typedef struct _HIDD_ATTRIBUTES {
    ULONG   Size; // = sizeof (struct _HIDD_ATTRIBUTES)
    USHORT  VendorID;
    USHORT  ProductID;
    USHORT  VersionNumber;
} HIDD_ATTRIBUTES, *PHIDD_ATTRIBUTES;

extern "C" {
BOOLEAN __stdcall
HidD_GetAttributes (
    __in  HANDLE              HidDeviceObject,
    __out PHIDD_ATTRIBUTES    Attributes
    );
}

HANDLE find( USHORT VendorID, USHORT ProductID )
{
	HANDLE write_handle ;
	HDEVINFO device_info_set = SetupDiGetClassDevs( &GUID_DEVINTERFACE_HID,
		NULL,
		NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	int device_index = 0;
	SP_DEVICE_INTERFACE_DATA device_interface_data;
	SP_DEVICE_INTERFACE_DETAIL_DATA* device_interface_detail_data ;
	memset( &device_interface_data, 0, sizeof(device_interface_data) );
	device_interface_data.cbSize = sizeof(device_interface_data);

	for(;;device_index++)	
	{
		DWORD required_size = 0;
		BOOL res;
		HIDD_ATTRIBUTES attrib;
		res = SetupDiEnumDeviceInterfaces(device_info_set,
			NULL,
			&GUID_DEVINTERFACE_HID,
			device_index,
			&device_interface_data);
		if ( !res) {			
			break;
		} 

		res = SetupDiGetDeviceInterfaceDetail(device_info_set,
			&device_interface_data,
			NULL,
			0,
			&required_size,
			NULL);

		device_interface_detail_data = (SP_DEVICE_INTERFACE_DETAIL_DATA*) malloc(required_size);
		device_interface_detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		res = SetupDiGetDeviceInterfaceDetail(device_info_set,
			&device_interface_data,
			device_interface_detail_data,
			required_size,
			NULL,
			NULL);		

		write_handle = CreateFile(device_interface_detail_data->DevicePath,
			GENERIC_WRITE | GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			//FILE_FLAG_OVERLAPPED,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (write_handle == INVALID_HANDLE_VALUE) {
			continue ;
		}

		attrib.Size = sizeof(HIDD_ATTRIBUTES);
		HidD_GetAttributes(write_handle, &attrib);
		if ( attrib.VendorID == VendorID && attrib.ProductID == ProductID )
			return write_handle ;
		else
			CloseHandle( write_handle );
	}
	return INVALID_HANDLE_VALUE ;
}

//write_handle = find( 0x1294, 0x1320 );
int hid_write( USHORT VendorID, USHORT ProductID, size_t size, char value )
{
	DWORD dwErr ;
	int device_index = 0;	
	HANDLE write_handle = INVALID_HANDLE_VALUE;
	HIDD_ATTRIBUTES attrib;			                	

	//printf("DevicePath : %s\n" , DevicePath );

	write_handle = find( VendorID, ProductID );

	if (write_handle == INVALID_HANDLE_VALUE) {
		printf("Unable to open err=0x%x\n", GetLastError() );
		return 1 ;
	}

	attrib.Size = sizeof(HIDD_ATTRIBUTES);
	HidD_GetAttributes(write_handle, &attrib);

	//wprintf(L"Product/Vendor: %04x %04x\n", attrib.ProductID, attrib.VendorID);

	{
		BOOL ret ;
		BYTE *buff = (BYTE*)malloc( size );
		DWORD written = 0 ;	
		memset( buff, 0, size );		
		buff[1] = value ;

		ret = WriteFile( write_handle, buff, 6, &written, NULL );
		if ( ret == FALSE ) {
			dwErr = GetLastError();
			return dwErr ;
		}		
	}

	CloseHandle( write_handle );
	return 0;
}


int main( int argc, char** argv )
{
	if ( argc < 2 ) {
		printf("program usage:\n\tprogram [0-7]\n");
		return -1 ;
	}
	char c = argv[1][0];
	c -= '0' ;
	return hid_write( 0x1294, 0x1320, 6, c );
}