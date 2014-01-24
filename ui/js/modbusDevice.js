var ModbusDevice = function (dh, deviceId, slaveId, cmdTimeout, notifTimeout) {
	this.deviceHive = dh;
	this.deviceId = deviceId;
	this.slaveId = slaveId;
	this.cmdTimeout = cmdTimeout;
	this.notifTimeout = notifTimeout;
};
	
ModbusDevice.prototype.sendCommand = function (name, parameters) {
	var commandResultDeferred = $.Deferred();

	setTimeout(function () {
		commandResultDeferred.reject({
			name: 'Command Timeout'
		});
	}, this.cmdTimeout);

	var request = this.deviceHive.sendCommand(this.deviceId, name, parameters);

	request.result(function(command) {
		if (command.status === 'success') {
			commandResultDeferred.resolve(command);
		} else {
			commandResultDeferred.reject({ name: 'failed', command: command });
		}
	});

	return commandResultDeferred;
};

ModbusDevice.prototype.readData = function (type, address, count) {
	var readDeferred = $.Deferred();

	setTimeout(function () {
		readDeferred.reject({
			name: 'Notification Timeout'
		});
	}, this.notifTimeout);

	this.deviceHive.notification(function (deviceId, parameters) {
		if (parameters.notification == 'MODBUS Slave Report') {
			readDeferred.resolve(parameters.parameters);
		}
	});

	this.sendCommand('read', {
		i: this.slaveId,
		t: type,
		a: address,
		c: count
	});

	return readDeferred.promise();
};

ModbusDevice.prototype.writeData = function (startAddress, values) {
	return this.sendCommand('write', {
		i: this.slaveId,
		a: startAddress,
		d: values
	});
};

ModbusDevice.prototype.writeValue = function (startAddress, value) {
	return this.sendCommand('write1', {
		a: startAddress,
		v: value
	});
};
