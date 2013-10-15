import usb.core
import usb.util
from sys import *
import array
import struct
import math
from intelhex import IntelHex
import argparse

class usbdev:
	def __init__(self):
		self.dev = usb.core.find(idVendor=0x03EB, idProduct=0x40F0)
		if self.dev is None:
			raise ValueError('BoardReplicator has not been found')
			
		self.dev.set_configuration()
		
	def readblock(self, fid, idx, len=256):
		ret = self.dev.ctrl_transfer(bmRequestType = 0xa0,
								bRequest = 0x10,
								wIndex = (((0xf & fid) << 12) | (idx & 0xfff)) ,
								data_or_wLength = len)
		return ret
		
	def writeblock(self, idx, data=[]):
		ret = self.dev.ctrl_transfer(bmRequestType = 0x20,
								bRequest = 0x12,
								wIndex = idx ,
								data_or_wLength = data)
		return ret

	def clearall(self):
		ret = self.dev.ctrl_transfer(bmRequestType = 0x20,
								bRequest = 0x11,
								wIndex = 0 ,
								data_or_wLength = [])
		return ret		
		
	def loadfw(self, fid=0):
		fw = brfirmware()
		fw.loadfromusb(self, fid)
		return fw

	def writefw(self, fw):
		fw.writetousb(self)

		

class brfirmware:
	def __init__(self):
		self.blocksize = 256
		pass
	
	def __str__(self):
		return "FW: name=%s fw_size=%d ee_size=%d fuses=%x:%x:%x" % (self.h_name,self.h_fw_size,self.h_ee_size,self.h_f,self.h_fh,self.h_fe)
	
	def loadfromusb(self, usbdev, fid = 0):
		header = usbdev.readblock(fid, 0)
		
		self.h_off,self.h_name,self.h_fw_size,self.h_ee_size,self.h_flags,self.h_lock,self.h_f,self.h_fh,self.h_fe = struct.unpack_from("<I16sHHBBBBB", header)
		
		# Download firmware
		fw_idxs = math.ceil(float(self.h_fw_size) / self.blocksize)
		ar = array.array('B', [])
		fw_len = self.h_fw_size
		fw_idx = 1
		while fw_len > 0:
			tmplen = 4096 if fw_len > 4096 else fw_len
			ar = ar + usbdev.readblock(fid, fw_idx, tmplen)
			fw_len -= tmplen
			fw_idx += int(math.ceil(float(tmplen) / self.blocksize))
		
		self.fw_data = ar
		
		#Download eeprom
		ee_idxs = math.ceil(float(self.h_ee_size) / self.blocksize)
		ar = array.array('B', [])
		ee_len = self.h_ee_size
		ee_idx = fw_idx
		while ee_len > 0:
			tmplen = 4096 if ee_len > 4096 else ee_len
			ar = ar + usbdev.readblock(fid, ee_idx, tmplen)
			ee_len -= tmplen
			ee_idx += int(math.ceil(float(tmplen) / self.blocksize))
		
		self.ee_data = ar
		
		print self.h_fw_size
		print "FW Size: %d  EE Size: %d  Fuses %02x:%02x:%02x  Lock %02x" % (self.h_fw_size, self.h_ee_size, self.h_f, self.h_fh, self.h_fe, self.h_lock)
		
	def writetousb(self, usbdev):
		hbuf = array.array('B', [ 0xff ] * self.blocksize)
		struct.pack_into("<I16sHHBBBBB", hbuf, 0, self.h_off,self.h_name,self.h_fw_size,self.h_ee_size,self.h_flags,self.h_lock,self.h_f,self.h_fh,self.h_fe)
		
		# send header
		usbdev.writeblock(0, hbuf)
		
		# send firmware
		fw_len = self.h_fw_size
		fw_idx = 1
		while fw_len > 0:
			tmplen = 4096 if fw_len > 4096 else fw_len
			wlen = usbdev.writeblock(fw_idx, self.fw_data[(fw_idx-1)*self.blocksize:(fw_idx-1)*self.blocksize+tmplen])
			fw_len -= tmplen
			fw_idx += int(math.ceil(float(tmplen) / self.blocksize))
			print "fw idx %d len %d => %d " % (fw_idx, tmplen, wlen)
			
		# send eeprom
		ee_len = self.h_ee_size
		ee_idx = fw_idx		
		while ee_len > 0:
			tmplen = 4096 if ee_len > 4096 else ee_len
			wlen = usbdev.writeblock(ee_idx, self.ee_data[(ee_idx-fw_idx)*self.blocksize:(ee_idx-fw_idx)*self.blocksize+tmplen])
			ee_len -= tmplen
			ee_idx += int(math.ceil(float(tmplen) / self.blocksize))
			print "ee idx %d len %d => %d" % (ee_idx,  tmplen, wlen)
			
		# send flush command
		usbdev.writeblock(0x0fff, [])
		
		
	def writetofile(self, fwname="out.hex", eename="out.eep"):
		ih = IntelHex()
		ih.start_address = 0x100000
		for x in xrange(len(self.fw_data)): ih[x] = self.fw_data[x] 
		ih.write_hex_file(fwname)
		
		ihe = IntelHex()
		ihe.start_address = 0x100000
		for x in xrange(len(self.ee_data)): ihe[x] = self.ee_data[x] 
		ihe.write_hex_file(eename)
		
		print "Name: %s FW Size: %d  EE Size: %d  Fuses %02x:%02x:%02x  Lock %02x" % (self.h_name, self.h_fw_size, self.h_ee_size, self.h_f, self.h_fh, self.h_fe, self.h_lock)
		
	def loadfromfile(self, fwname="Tamer.hex", eename="Tamer.eep", name="clocktamer", fuses="5e:d8:f4"):

#	def loadfromfile(self, fwname="Tamer.hex", eename="Tamer.eep", name="clocktamer", fuses="5e:d9:f4"):
		ih = IntelHex(fwname)
		ihe = IntelHex(eename)
		
		self.h_off = 0xffff
		self.h_name = name
		self.h_fw_size = int(math.ceil(float(ih.maxaddr()) / self.blocksize))* self.blocksize
		self.h_ee_size = int(math.ceil(float(ihe.maxaddr()) / self.blocksize))* self.blocksize
		
		self.h_flags = 0
		self.h_lock = 0xff
		self.h_f  = int(fuses[0:2], 16)
		self.h_fh = int(fuses[3:5], 16)
		self.h_fe = int(fuses[6:8], 16)
		
		self.fw_data = array.array('B', [ ih[i] for i in xrange(self.h_fw_size) ]) 
		self.ee_data = array.array('B', [ ihe[i] for i in xrange(self.h_ee_size) ])
		
		
def main():
	parser = argparse.ArgumentParser(description='Update BoardReplicaot flashing firmwares')
	parser.add_argument('-r', '--read', help='Read firmware form Boardreplicator', action='store_true')
	parser.add_argument('-c', '--clear', help='Clear all firmwares on BoardReplicator', action='store_true')
	parser.add_argument('-w', '--write', help='Append firmware', action='store_true')
	args = parser.parse_args()

	if ((not args.read) and (not args.clear) and (not args.write)):
		parser.print_help()
	
	usb = usbdev()
	if (args.read):
		print "Reading firmware..."
		usb.loadfw().writetofile()
		print " ... done"
		
	if (args.clear):
		print "Reset all firmwares..."
		usb.clearall()
		print " ... done"
	
	if (args.write):
		print "Downloading new firmware..."
		fw = brfirmware()
		fw.loadfromfile()
		fw.writetousb(usb)
		print " ... done"	
	

if __name__ == "__main__":
	main()
	
