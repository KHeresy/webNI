function Joint2D(x,y,c) {
	this.x = x;
	this.y = y;
	this.c = x;
}

function WebNI() {
	//public  member function

	// connect with remote server
	//	sURI		WebSocket Server URI, ex: ws://localhost:9002/
	//	onDoneFunc	Callback function when done. 
	//				This function will get two result, this WebNI object and an array [w,h]
	this.open = function (sURI, onDoneFunc) {
		var pThis = this;

		// Open WebSocket Connection
		pThis.sStatus = "initializing";
		pThis.wsNIServer = new WebSocket(sURI);
		pThis.wsNIServer.binaryType = "arraybuffer";

		pThis.wsNIServer.onopen = function (evt) {
			pThis.sStatus = "connected";
			pThis.getDepthSize(onDoneFunc);
		};
	}

	// close connection
	this.close = function () {
		this.wsNIServer.close();
	}

	// get current user list
	//	onDoneFunc	Callback function when done. 
	//				This function will get two result, this WebNI object and an array of UserID
	this.getUserList = function (onDoneFunc) {
		var pThis = this;
		pThis.sStatus = "get user_list";
		pThis.wsNIServer.onmessage = function (msg) {
			if (msg.data instanceof ArrayBuffer) {
				pThis.aUserList = new Uint16Array(msg.data);
				if (pThis.aUserList.length > 0 && pThis.aUserList[0] != 0) {
					pThis.sStatus = "get user_list done";
					onDoneFunc(pThis, pThis.aUserList);
				}
				else {
					pThis.sStatus = "no user found";
					onDoneFunc(pThis, new Array());
				}
			}
			else {
				pThis.sStatus = msg.data;
			}
		}
		pThis.wsNIServer.send("get user_list");
	}

	// get user 2D skeleton data
	//	userId		The id of user to get skeleton, should get from getUserList()
	//	onDoneFunc	Callback function when done. 
	//				This function will get two result, this WebNI object and an array of skeleton data.
	//				The skeleton array use string as key, each joint has 3 value x, y, c.
	//				If no skeleton can be get, the second value will be null
	this.getUserSkeleton2D = function (userId, onDoneFunc) {
		var pThis = this;
		pThis.sStatus = "get user skeleton";
		pThis.wsNIServer.onmessage = function (msg) {
			if (msg.data instanceof ArrayBuffer) {
				var aSkletonArray = new Float32Array(msg.data);

				var mSkeleton = new Array();
				mSkeleton['head'] = new Joint2D( aSkletonArray[0], aSkletonArray[1], aSkletonArray[2] );
				mSkeleton['neck'] = new Joint2D( aSkletonArray[3], aSkletonArray[4], aSkletonArray[5] );
				mSkeleton['lshoulder'] = new Joint2D(aSkletonArray[6], aSkletonArray[7], aSkletonArray[8]);
				mSkeleton['rshoulder'] = new Joint2D(aSkletonArray[9], aSkletonArray[10], aSkletonArray[11]);
				mSkeleton['lelbow'] = new Joint2D(aSkletonArray[12], aSkletonArray[13], aSkletonArray[14]);
				mSkeleton['relbow'] = new Joint2D(aSkletonArray[15], aSkletonArray[16], aSkletonArray[17]);
				mSkeleton['lhand'] = new Joint2D(aSkletonArray[18], aSkletonArray[19], aSkletonArray[20]);
				mSkeleton['rhand'] = new Joint2D(aSkletonArray[21], aSkletonArray[22], aSkletonArray[23]);
				mSkeleton['torso'] = new Joint2D(aSkletonArray[24], aSkletonArray[25], aSkletonArray[26]);
				mSkeleton['lhip'] = new Joint2D(aSkletonArray[27], aSkletonArray[28], aSkletonArray[29]);
				mSkeleton['rhip'] = new Joint2D(aSkletonArray[30], aSkletonArray[31], aSkletonArray[32]);
				mSkeleton['lknee'] = new Joint2D(aSkletonArray[33], aSkletonArray[34], aSkletonArray[35]);
				mSkeleton['rknee'] = new Joint2D(aSkletonArray[36], aSkletonArray[37], aSkletonArray[38]);
				mSkeleton['lfoot'] = new Joint2D(aSkletonArray[39], aSkletonArray[40], aSkletonArray[41]);
				mSkeleton['rfoot'] = new Joint2D(aSkletonArray[42], aSkletonArray[43], aSkletonArray[44]);
				onDoneFunc(pThis, mSkeleton);
			}
			else {
				pThis.sStatus = msg.data;
				onDoneFunc(pThis, null);
			}
		}
		pThis.wsNIServer.send("get user " + userId + " skeleton 2D");
	}

	this.setOnClose = function (onClodeFunc) {
		var pThis = this;
		pThis.wsNIServer.onclose = function (evt) {
			onClodeFunc();
		}
	}

	// member data
	this.sStatus;
	this.DepthSize;
	this.aUserList;

	// internal function
	this.getDepthSize = function (onDoneFunc) {
		var pThis = this;
		// get the size of depth map
		pThis.sStatus = "get depth size";
		pThis.wsNIServer.onmessage = function (msg) {
			if (msg.data instanceof ArrayBuffer) {
				pThis.DepthSize = new Uint16Array(msg.data);
				pThis.sStatus = "get depth size done";
			}
			else {
				pThis.sStatus = "get depth size failed"
			}
			onDoneFunc(pThis, pThis.DepthSize);
		};
		pThis.wsNIServer.send("get depth_size");
	}

	// internal data
	this.wsNIServer;
}
