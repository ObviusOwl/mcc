
var express = require('express');
var io = require('socket.io');
var net = require('net');
var http = require('http');

var fs = require('fs');

var myapp = function(){

	this.servers = {
		
	}

	this.server_setRoutes = function(){
		var s = this.servers.express;
		s.get('*', function(req, res, next){
			//console.log( "GET "+req.url );
			next();
		});

		s.get('/', function(req, res){
			res.sendFile(__dirname + '/index.html');
		});
		s.get('/client.css', function(req, res){
			res.sendFile(__dirname + '/client.css');
		});
		s.get('/client.js', function(req, res){
			res.sendFile(__dirname + '/client.js');
		});

		s.use( "/bower_components", express.static(__dirname+"/bower_components") )

	}
	
	this.connectSocket = function(){
		var self = this;
		this.servers.socket = net.createConnection("/home/jojo/blaa");
		this.servers.socket.on("data", function(data) {
			self.servers.io.emit('console',data.toString() );
		});
		this.servers.socket.on("close", function(data) {
			console.log("connection to socket failed, retrying in 4s");
			setTimeout( function(){self.connectSocket();}, 4000 );
		});
		this.servers.socket.on("error", function(e) {
			if( e.errno == 'ECONNREFUSED' ){
			}
		});			
		this.servers.socket.on("connect", function(e) {
			console.log("socket connected");
		});
	}
	
	this.startServers = function(){
		var self = this;
		this.servers.express = express();
		this.server_setRoutes();

		this.servers.http = http.Server( this.servers.express );
		this.servers.http.listen(3000, function(){
			console.log('listening on *:3000');
		});

		this.connectSocket();

		this.servers.io = io( this.servers.http );
		this.servers.io.on('connection', function(socket){
			socket.on('console', function(msg){
				//console.log("sending msg: "+decodeURI(msg))
				//self.servers.socket.write( "msg"+decodeURI(msg) );
				self.servers.socket.write( msg );
			});
		});
	}
	
	this.run = function(){
		this.startServers();
	}
	
	return this;
}

var a = myapp();

a.run();