(function () {
	var config = {
		serverUrl: "[ServerUrl]",
		login: "[Login]",
		password: "[Password]",
		deviceId: "[DeviceId]",
		slaveId: 1,
		commandTimeout: 200000,
		notificationTimeout: 300000,
		readRegCountMax: 10
	};

	var deviceHive = new DeviceHive(config.serverUrl, config.login, config.password);

	var initializeApp = function () {
		var readRegAdElement = $('#read-reg-ad'),
			readRegCountElement = $('#read-reg-count'),
		    readRegTypeElement = $('#read-reg-type'),
			readDataElement = $('.read-data'),
			writeRegVal = $('.write-reg-val'),
			writeForm = $('#form-write'),
			readForm = $('#form-read'),
			addRegValBtn = $('.btn-add-reg-val');

		var device = new ModbusDevice(deviceHive, config.deviceId, config.slaveId, config.commandTimeout, config.notificationTimeout);
		deviceHive.subscribe(config.deviceId);

		var formValidationRules = {
			errorClass: 'has-error',
			highlight: function (element, errorClass) {
				$(element).parents('.form-group').addClass(errorClass);
			},
			unhighlight: function (element, errorClass) {
				$(element).parents('.form-group').removeClass(errorClass);
				$(element).tooltip('destroy');
			},
			errorPlacement: function (error, element) {
				element.tooltip('destroy');
				element.tooltip({
					html: error.html(),
					title: error.text()
				});
				element.tooltip('show');
			},
			rules: {
				startAddress: {
					required: true
				},
				registersCount: {
					required: true,
					min: 1,
					max: 10
				}
			},
			messages: {
				startAddress: {
					required: "Please specify starting address"
				},
				registersCount: {
					required: 'Please specify registers count',
					min: 'Count should be more then 1',
					max: 'This prototype supports reading of maximum 10 registers'
				}
			}
		};

		writeForm.validate(formValidationRules);
		readForm.validate(formValidationRules);

		var hideSuccessTimeout, hideErrorTimeout;
		var processFormCommand = function (form, successAlert, errorAlert, loader, processCallback) {
			if (!form.valid()) {
				return;
			}

			var fieldset = form.find('fieldset');

			clearTimeout(hideSuccessTimeout);
			clearTimeout(hideErrorTimeout);
			fieldset.attr('disabled', 'disabled');
			$.when(successAlert.slideUp(100).promise(), errorAlert.slideUp(100).promise()).then(function () {
				return loader.fadeIn().promise();
			}).then(function () {
				return processCallback();
			}).done(function () {
				return successAlert.slideDown();
			}).fail(function (err) {
				errorAlert.find('span').text(err.name);
				return errorAlert.slideDown();
			}).always(function () {
				loader.fadeOut().promise();
				hideSuccessTimeout = setTimeout(function () {
					successAlert.slideUp();
				}, 3500);
				hideErrorTimeout = setTimeout(function () {
					errorAlert.slideUp();
				}, 10000);
				fieldset.removeAttr('disabled');
			});
		};

		$('.btn-write').click(function (ev) {
			ev.preventDefault();

			var startingAddress = parseInt($('#write-reg-ad').val(), 16);
			var valElements = $('.write-reg-val').each(function () {
				if (!$(this).val()) {
					$(this).val('0');
				}
			});

			var values = $.map(valElements, function (el) {
				return parseInt(el.value, 16);
			});

			processFormCommand(writeForm, $('.alert-write.alert-success'), $('.alert-write.alert-danger'), $('.loader-write'), function () {
				return device.writeData(startingAddress, values);
			});

			return false;
		});

		$('.btn-read').click(function (ev) {
			ev.preventDefault();

			var addressVal = readRegAdElement.val();
			var regCountVal = readRegCountElement.val();

			var type = parseInt(readRegTypeElement.find(':selected').val());
			var startingAddress = parseInt(addressVal, 16);
			var count = parseInt(regCountVal);

			processFormCommand(readForm, $('.alert-read.alert-success'), $('.alert-read.alert-danger'), $('.loader-read'), function () {
				return readDataElement.slideUp().promise().then(function () {
					return device.readData(type, startingAddress, count);
				}).then(function (data) {
					readDataElement.html('');
					for (var v in data.d.v) {
						var val = data.d.v[v];
						var valStr = (val >>> 0).toString(16);
						if (valStr.length > 4) {
							valStr = valStr.slice(valStr.length - 4, valStr.length);
						}
						var elem = $('<span>').text('0x' + valStr);
						readDataElement.append(elem);
					}

					return readDataElement.slideDown().promise();
				});
			});

			return false;
		});

		$('.reg-address').keyfilter($.fn.keyfilter.defaults.masks.hex);
		writeRegVal.keyfilter($.fn.keyfilter.defaults.masks.hex);
		readRegCountElement.keyfilter($.fn.keyfilter.defaults.masks.num);

		readRegTypeElement.change(function () {
			$('.reg-address-prefix').text($(this).find(':selected').val());
		});

		var validateRegValCount = function () {
			if ($('.write-reg-val').length >= 10) {
				addRegValBtn.attr('disabled', 'disabled');
			} else {
				addRegValBtn.removeAttr('disabled');
			}
		};

		addRegValBtn.click(function () {
			var elem = $($('#value-tmpl').html());
			var input = elem.find('input');
			input.attr('name', input.attr('name') + ($('.write-reg-val').length + 1));
			$('.reg-values').append(elem);
			input.rules('add', {
				required: true,
				messages: {
					required: "Please specify a value or remove this register",
				}
			});
			elem.keyfilter($.fn.keyfilter.defaults.masks.hex);
			validateRegValCount();
		});

		$(document).on("click", '.btn-remove-reg-val', function () {
			$(this).parents('.form-group').remove();
			validateRegValCount();
		});
	};

	var documenReadyDeferred = $.Deferred();
	$(document).ready(documenReadyDeferred.resolve);
	documenReadyDeferred.promise().then(function () {
		return deviceHive.openChannel();
	}).then(function () {
		return initializeApp();
	}).then(function () {
		return $('.main-loader').fadeOut().promise();
	}).then(function () {
		return $('.main').fadeIn().promise();
	});
})()