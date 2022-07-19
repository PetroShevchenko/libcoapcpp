#!/usr/bin/env python3
# -*- coding: utf-8 -*- 
import argparse
import socket
import sys

try:
	import wx
	import wx.xrc
except:
	print("Please install wxPython Phoenix and all its dependencies to use GUI mode")
	print("sudo apt install libgtk-3-dev python3-pip")
	print("pip install -U -f https://extras.wxpython.org/wxPython4/extras/linux/gtk3/ubuntu-20.04 wxPython")
	sys.exit()

class UdpClient:
	def __init__(self, ipv4addr, port):
		self.addr = ipv4addr
		self.port = int(port)
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

	def send(self, data):
		self.sock.sendto(data.encode(), (self.addr, self.port))

	def recv(self):
		data, addr = self.sock.recvfrom(self.port)
		data = data.decode()
		return data, addr

class UdpClientFrame ( wx.Frame ):
	
	def __init__( self, parent ):
		wx.Frame.__init__ ( self, parent, id = wx.ID_ANY, title = u"Simple UDP client", pos = wx.DefaultPosition, size = wx.Size( 450,400 ), style = wx.DEFAULT_FRAME_STYLE|wx.TAB_TRAVERSAL )
		
		self.SetSizeHints( wx.DefaultSize, wx.DefaultSize )
		
		bSizer1 = wx.BoxSizer( wx.VERTICAL )
		
		bSizer3 = wx.BoxSizer( wx.HORIZONTAL )
		
		self.m_staticText3 = wx.StaticText( self, wx.ID_ANY, u"Destination IPv4:", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText3.Wrap( -1 )
		bSizer3.Add( self.m_staticText3, 0, wx.ALL, 5 )
		
		self.m_textCtrl3 = wx.TextCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer3.Add( self.m_textCtrl3, 1, wx.ALL, 5 )
		
		self.m_staticText4 = wx.StaticText( self, wx.ID_ANY, u"Port:", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText4.Wrap( -1 )
		bSizer3.Add( self.m_staticText4, 0, wx.ALL, 5 )
		
		self.m_textCtrl4 = wx.TextCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer3.Add( self.m_textCtrl4, 0, wx.ALL, 5 )
		
		
		bSizer1.Add( bSizer3, 1, wx.EXPAND, 5 )
		
		bSizer4 = wx.BoxSizer( wx.VERTICAL )
		
		self.m_staticText5 = wx.StaticText( self, wx.ID_ANY, u"Received:", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText5.Wrap( -1 )
		bSizer4.Add( self.m_staticText5, 0, wx.ALL, 5 )
		
		self.m_textCtrl31 = wx.TextCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.Size( 450,150 ), wx.TE_MULTILINE )
		bSizer4.Add( self.m_textCtrl31, 0, wx.ALL, 5 )
		
		
		bSizer1.Add( bSizer4, 2, wx.EXPAND, 5 )
		
		bSizer5 = wx.BoxSizer( wx.VERTICAL )
		
		self.m_staticText6 = wx.StaticText( self, wx.ID_ANY, u"To be sent:", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText6.Wrap( -1 )
		bSizer5.Add( self.m_staticText6, 0, wx.ALL, 5 )
		
		self.m_textCtrl41 = wx.TextCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.Size( 450,-1 ), 0 )
		bSizer5.Add( self.m_textCtrl41, 0, wx.ALL, 5 )
		
		
		bSizer1.Add( bSizer5, 2, wx.EXPAND, 5 )
		
		bSizer6 = wx.BoxSizer( wx.VERTICAL )
		
		self.m_button1 = wx.Button( self, wx.ID_ANY, u"Send", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer6.Add( self.m_button1, 0, wx.ALL|wx.ALIGN_CENTER_HORIZONTAL, 5 )
		
		
		bSizer1.Add( bSizer6, 1, wx.EXPAND, 5 )
		
		
		self.SetSizer( bSizer1 )
		self.Layout()
		
		self.Centre( wx.BOTH )
		
		# Connect Events
		self.m_button1.Bind( wx.EVT_LEFT_UP, self.m_button1OnLeftDown )
	
	def __del__( self ):
		pass

	def m_button1OnLeftDown( self, event ):
		event.Skip()
		addr = self.m_textCtrl3.GetLineText(0)
		port = int(self.m_textCtrl4.GetLineText(0))
		request = self.m_textCtrl41.GetLineText(0)
		client = UdpClient(addr, port)
		client.send(request)
		response, clientAddr = client.recv()
		self.m_textCtrl31.write(response)
	

def gui_client():
	app = wx.App()
	frame = UdpClientFrame(None)
	frame.Show()
	app.MainLoop()

def cli_client(addr, port):
	quit = False
	client = UdpClient(addr, port)
	while not quit:
		request = input("> ")
		if request == "quit":
			quit = True
			continue
		client.send(request)
		response, addr = client.recv()
		print("<-- (", addr, ") :")
		print(response)

def main():
	parser = argparse.ArgumentParser()
	parser.add_argument('--gui', help='use GUI mode', action="store_true")
	parser.add_argument('-a', help='IPv4 address')
	parser.add_argument('-p', help='port number')
	args = parser.parse_args()
	if args.gui == False:
		if args.a == None or  args.p == None:
			parser.print_help()
			sys.exit()
		cli_client(args.a, args.p)
	else:
		gui_client()

if __name__=='__main__':
	main()
