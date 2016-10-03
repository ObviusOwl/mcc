$(function(){
	var socket = io();
	socket.on("console",function(msg){
		var line_re = /(.*)\n/gm
		var line = "";
		var con_a = [];
		var con_s =  $('#console').html();
		while( line != null ){
			line = line_re.exec( con_s );
			if( line != null ){
				con_a.push( line[1] );
			}
		}
		if( con_a.length > 50 ){
			con_a.splice( 0, con_a.length - 50 );
			$("#console").html( con_a.join("\n") );
		}
		
		$("#console").append( msg );
		var element = document.getElementById("console");
		var a = element.scrollHeight - element.scrollTop;
		var b = element.clientHeight;
		if( Math.abs((b/a)-1) < 0.4 ){
			element.scrollTop = element.scrollHeight;
		}
	});
	
	$("#prompt").on( 'keydown', function(evt){
		if( evt.which == 13 ){
			socket.emit('console', $("#prompt").text() );
			//socket.emit('console', encodeURI($("#prompt").text()) );
			//console.log( "sending "+ encodeURI($("#prompt").text()) )
			$("#prompt").html("");
		}
	})
})
