.pragma library
.import QtQuick as QtQuick
.import Linphone as Linphone


// Orange
	var main = {
		color: {
			get normal() { return Linphone.DefaultStyle.main1_500_main },
			get hovered() { return Linphone.DefaultStyle.main1_600 },
			get pressed() { return Linphone.DefaultStyle.main1_700 }
		},
		text: {
			get normal() { return Linphone.DefaultStyle.grey_0 },
			get pressed() { return Linphone.DefaultStyle.grey_0 }
		},
		image: {
			get normal() { return Linphone.DefaultStyle.grey_0 },
			get pressed() { return Linphone.DefaultStyle.grey_0 }
		}
	}
	var mainLightBorder =  Object.assign({
			borderColor : {
				get keybaordFocused() { return Linphone.DefaultStyle.main2_0 }
			}
		}, main)

// White with orange border
	var secondary = {
		color: {
			get normal() { return Linphone.DefaultStyle.surface },
			get hovered() { return Linphone.DefaultStyle.main1_100 },
			get pressed() { return Linphone.DefaultStyle.main1_500_main }
		},
		borderColor: {
			get normal() { return Linphone.DefaultStyle.main1_500_main },
			get keybaordFocused() { return Linphone.DefaultStyle.main2_900 }
		},
		text: {
			get normal() { return Linphone.DefaultStyle.main1_500_main },
			get pressed() { return Linphone.DefaultStyle.grey_0 }
		},
		image: {
			get normal() { return Linphone.DefaultStyle.main1_500_main },
			get pressed() { return Linphone.DefaultStyle.grey_0 }
		}
	}

	// White with orange icon
	var player = {
		color: {
			get normal() { return Linphone.DefaultStyle.surface },
			get hovered() { return Linphone.DefaultStyle.main1_100 },
			get pressed() { return Linphone.DefaultStyle.main1_500_main }
		},
		text: {
			get normal() { return Linphone.DefaultStyle.main1_500_main },
			get pressed() { return Linphone.DefaultStyle.main1_500_main }
		},
		image: {
			get normal() { return Linphone.DefaultStyle.main1_500_main },
			get pressed() { return Linphone.DefaultStyle.main1_500_main }
		}
	}

// Light orange
	var tertiary = {
		color: {
			get normal() { return Linphone.DefaultStyle.main1_100 },
			get hovered() { return Linphone.DefaultStyle.main1_200 },
			get pressed() { return Linphone.DefaultStyle.main1_300 }
		},
		text: {
			get normal() { return Linphone.DefaultStyle.main1_500_main },
			get pressed() { return Linphone.DefaultStyle.main1_500_main }
		},
		image: {
			get normal() { return Linphone.DefaultStyle.main1_500_main },
			get pressed() { return Linphone.DefaultStyle.main1_500_main }
		}
	}

// Blue-grey
	var grey = {
		color: {
			get normal() { return Linphone.DefaultStyle.main2_200 },
			get hovered() { return Linphone.DefaultStyle.main2_300 },
			get pressed() { return Linphone.DefaultStyle.main2_400 }
		},
		text: {
			get normal() { return Linphone.DefaultStyle.main2_500_main },
			get pressed() { return Linphone.DefaultStyle.main2_700 }
		},
		image: {
			get normal() { return Linphone.DefaultStyle.main2_500_main },
			get pressed() { return Linphone.DefaultStyle.main2_700 }
		}
	}

// Red phone
	var phoneRed = {
		get iconSource() { return Linphone.AppIcons.endCall },
		color: {
			get normal() { return Linphone.DefaultStyle.danger_500_main },
			get hovered() { return Linphone.DefaultStyle.danger_700 },
			get pressed() { return Linphone.DefaultStyle.danger_900 }
		},
		text: {
			get normal() { return Linphone.DefaultStyle.grey_0 },
			get pressed() { return Linphone.DefaultStyle.grey_0 }
		},
		image: {
			get normal() { return Linphone.DefaultStyle.grey_0 },
			get pressed() { return Linphone.DefaultStyle.grey_0 }
		}
	}
	var phoneRedLightBorder =  Object.assign({
			borderColor : {
				get keybaordFocused() { return Linphone.DefaultStyle.main2_0 }
			}
		}, phoneRed)

// Green phone
	var phoneGreen = {
		get iconSource() { return Linphone.AppIcons.phone },
		color: {
			get normal() { return Linphone.DefaultStyle.success_500_main },
			get hovered() { return Linphone.DefaultStyle.success_700 },
			get pressed() { return Linphone.DefaultStyle.success_900 }
		},
		text: {
			get normal() { return Linphone.DefaultStyle.grey_0 },
			get pressed() { return Linphone.DefaultStyle.grey_0 }
		},
		image: {
			get normal() { return Linphone.DefaultStyle.grey_0 },
			get pressed() { return Linphone.DefaultStyle.grey_0 }
		}
	}
	var phoneGreenLightBorder =  Object.assign({
		borderColor : {
			get keybaordFocused() { return Linphone.DefaultStyle.main2_0 }
		}
	}, phoneGreen)

// Checkable
	var checkable = {
		color: {
			get normal() { return Linphone.DefaultStyle.grey_500 },
			get hovered() { return Linphone.DefaultStyle.grey_600 },
			get pressed() { return Linphone.DefaultStyle.main2_400 },
			get hovered() { return Linphone.DefaultStyle.main2_400 }
		},
		text: {
			get normal() { return Linphone.DefaultStyle.grey_0 },
			get pressed() { return Linphone.DefaultStyle.grey_0 }
		},
		borderColor: {
			get keybaordFocused() { return Linphone.DefaultStyle.main2_0 }
		},
		image: {
			get normal() { return Linphone.DefaultStyle.grey_0 },
			get pressed() { return Linphone.DefaultStyle.grey_0 },
			get checked() { return Linphone.DefaultStyle.grey_0 }
		}
	}

// No background light
	var noBackgroundLight = {
		color: {
			normal: "#00000000",
			hovered: "#00000000",
            pressed: "#00000000",
			checked: "#00000000"
		},
		borderColor: {
			normal: "#00000000",
			get keybaordFocused() { return Linphone.DefaultStyle.main2_900 }
		},
		text: {
			get normal() { return Linphone.DefaultStyle.main2_200 },
			get hovered() { return Linphone.DefaultStyle.main2_300 },
            get pressed() { return Linphone.DefaultStyle.main2_400 },
            get checked() { return Linphone.DefaultStyle.main1_500_main }
		},
		image: {
			get normal() { return Linphone.DefaultStyle.main2_200 },
			get hovered() { return Linphone.DefaultStyle.main2_300 },
            get pressed() { return Linphone.DefaultStyle.main2_400 },
            get checked() { return Linphone.DefaultStyle.main1_500_main },
		}
	}

// No background
	var noBackground = {
		color: {
			normal: "#00000000",
			hovered: "#00000000",
            pressed: "#00000000",
			checked: "#00000000"
		},
		borderColor: {
			normal: "#00000000",
			get keybaordFocused() { return Linphone.DefaultStyle.main2_900 }
		},
		text: {
			get normal() { return Linphone.DefaultStyle.main2_600 },
			get hovered() { return Linphone.DefaultStyle.main2_700 },
            get pressed() { return Linphone.DefaultStyle.main2_800 },
            get checked() { return Linphone.DefaultStyle.main1_500_main }
		},
		image: {
			get normal() { return Linphone.DefaultStyle.main2_600 },
			get hovered() { return Linphone.DefaultStyle.main2_700 },
            get pressed() { return Linphone.DefaultStyle.main2_800 },
            get checked() { return Linphone.DefaultStyle.main1_500_main },
		}
	}
	var noBackgroundLightBorder =  Object.assign({
			borderColor : {
				get keybaordFocused() { return Linphone.DefaultStyle.main2_0 }
			}
		}, noBackground)

// No background red
	var noBackgroundRed = {
		color: {
			normal: "#00000000",
			hovered: "#00000000",
			pressed: "#00000000"
		},
		text: {
			get normal() { return Linphone.DefaultStyle.danger_500_main },
			get hovered() { return Linphone.DefaultStyle.danger_700 },
			get pressed() { return Linphone.DefaultStyle.danger_900 }
		},
		image: {
			get normal() { return Linphone.DefaultStyle.danger_500_main },
			get hovered() { return Linphone.DefaultStyle.danger_700 },
			get pressed() { return Linphone.DefaultStyle.danger_900 },
			get checked() { return Linphone.DefaultStyle.danger_900 }
		}
	}

// No background orange
	var noBackgroundOrange = {
		color: {
			normal: "#00000000",
			hovered: "#00000000",
			pressed: "#00000000"
		},
		text: {
			get normal() { return Linphone.DefaultStyle.main1_500_main },
			get hovered() { return Linphone.DefaultStyle.main1_600 },
			get pressed() { return Linphone.DefaultStyle.main1_700 }
		},
		image: {
			get normal() { return Linphone.DefaultStyle.main1_500_main },
			get hovered() { return Linphone.DefaultStyle.main1_600 },
			get pressed() { return Linphone.DefaultStyle.main1_700 }
		}
	}

	// Popup button
	var popupButton = {
		color: {
			normal: "#00000000",
			get hovered() { return Linphone.DefaultStyle.grey_100 },
			get pressed() { return Linphone.DefaultStyle.main2_300 }
		},
		text: {
			get normal() { return Linphone.DefaultStyle.main2_600 },
			get hovered() { return Linphone.DefaultStyle.main2_600 },
			get pressed() { return Linphone.DefaultStyle.main2_600 }
		},
		image: {
			get normal() { return Linphone.DefaultStyle.main2_600 },
			get hovered() { return Linphone.DefaultStyle.main2_600 },
			get pressed() { return Linphone.DefaultStyle.main2_600 }
		}
	}

// Icon + label button
	var hoveredBackground = {
		color: {
			normal: "#00000000",
			get hovered() { return Linphone.DefaultStyle.main2_100 },
			get pressed() { return Linphone.DefaultStyle.main2_100 }
		},
		text: {
			get normal() { return Linphone.DefaultStyle.main2_500_main },
			get hovered() { return Linphone.DefaultStyle.main2_500_main },
			get pressed() { return Linphone.DefaultStyle.main2_500_main }
		},
		image: {
			get normal() { return Linphone.DefaultStyle.main2_500_main },
			get hovered() { return Linphone.DefaultStyle.main2_500_main },
			get pressed() { return Linphone.DefaultStyle.main2_500_main }
		}
	}

// Icon + label red button
	var hoveredBackgroundRed = {
		color: {
			normal: "#00000000",
			get hovered() { return Linphone.DefaultStyle.main2_100 },
			get pressed() { return Linphone.DefaultStyle.main2_100 }
		},
		text: {
			get normal() { return Linphone.DefaultStyle.danger_500_main },
			get hovered() { return Linphone.DefaultStyle.danger_700 },
			get pressed() { return Linphone.DefaultStyle.danger_900 }
		},
		image: {
			get normal() { return Linphone.DefaultStyle.danger_500_main },
			get hovered() { return Linphone.DefaultStyle.danger_700 },
			get pressed() { return Linphone.DefaultStyle.danger_900 }
		}
	}

// Numpad
	var numericPad = {
		color: {
			get normal() { return Linphone.DefaultStyle.surface },
			get hovered() { return Linphone.DefaultStyle.grey_200 },
			get pressed() { return Linphone.DefaultStyle.grey_300 }
		},
		text: {
			get normal() { return Linphone.DefaultStyle.main2_600 },
			get pressed() { return Linphone.DefaultStyle.main2_700 }
		}
	}

// Green toast
	var toast = {
		color: {
			get normal() { return Linphone.DefaultStyle.surface },
			get hovered() { return Linphone.DefaultStyle.surface },
			get pressed() { return Linphone.DefaultStyle.surface }
		},
		borderColor: {
			get normal() { return Linphone.DefaultStyle.success_500_main },
			get keybaordFocused() { return Linphone.DefaultStyle.main2_900 }
		},
		text: {
			get normal() { return Linphone.DefaultStyle.success_500_main },
			get pressed() { return Linphone.DefaultStyle.success_700 }
		}
	}

// Security blue toast
	var securityToast = {
		color: {
			get normal() { return Linphone.DefaultStyle.surface },
			get hovered() { return Linphone.DefaultStyle.surface },
			get pressed() { return Linphone.DefaultStyle.surface }
		},
		borderColor: {
			get normal() { return Linphone.DefaultStyle.info_500_main },
			get keybaordFocused() { return Linphone.DefaultStyle.main2_900 }
		},
		text: {
			get normal() { return Linphone.DefaultStyle.info_500_main },
			get pressed() { return Linphone.DefaultStyle.info_500_main }
		}
	}

// Security red toast
	var securityToastError = {
		color: {
			get normal() { return Linphone.DefaultStyle.surface },
			get hovered() { return Linphone.DefaultStyle.surface },
			get pressed() { return Linphone.DefaultStyle.surface }
		},
		borderColor: {
			get normal() { return Linphone.DefaultStyle.danger_500_main },
			get keybaordFocused() { return Linphone.DefaultStyle.main2_900 }
		},
		text: {
			get normal() { return Linphone.DefaultStyle.danger_500_main },
			get pressed() { return Linphone.DefaultStyle.danger_500_main }
		}
	}

// White selected
	var whiteSelected = {
		color: {
			get normal() { return Linphone.DefaultStyle.surface },
			get hovered() { return Linphone.DefaultStyle.main2_100 },
			get selected() { return Linphone.DefaultStyle.grey_200 },
		},
		borderColor: {
			get keybaordFocused() { return Linphone.DefaultStyle.main2_900 }
		}
	}

	var hoveredBackgroundBis = {
		color: {
			normal: "#00000000",
			get hovered() { return Linphone.DefaultStyle.main2_100 },
			get pressed() { return Linphone.DefaultStyle.main2_100 }
		}
	}
