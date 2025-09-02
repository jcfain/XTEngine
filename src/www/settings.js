const systemInfoTab = document.getElementById("systemInfoTab");

Settings = {
    loaded: false,
    SettingProfile: {
        System: 0
    },
    FormControlTypes: {
        Double: 0,
        Int: 1,
        Text: 2,
        Radio: 3,
        Combo: 4,
        Checkbox: 5,
        DateTime: 6,
        Date: 7,
        Time: 8
    },
    debounceer: {},
    debounceerTimeout: 1000,
    createForm(group) {
        let div = document.createElement('div');
        div.id = group;
        div.classList.add("form");
        div.classList.add("tab-content-settings-section");
        let h2 = document.createElement('h3');
        h2.innerText = capitalizeFirstLetter(group);
        div.appendChild(h2);
        return div;
    },
    createFormControl(labelText, type, id, value) {
        let div = document.createElement('div');
        div.classList.add("formElement", "formElement--label-large");
        let labelElement = document.createElement('label');
        labelElement.setAttribute("title", labelText)
        labelElement.textContent = labelText;
        labelElement.setAttribute("for", id);
        let control;
        switch(type) {
            case this.FormControlTypes.Double:
                control = this.createDoubleInput(id, value);
                break;
            case this.FormControlTypes.Int:
                control = this.createNumberInput(id, value);
                break;
            case this.FormControlTypes.Text:
                control = this.createTextInput(id, value);
                break;
            case this.FormControlTypes.Radio:
                control = this.createRadioInput(id, value);
            break;
            case this.FormControlTypes.Combo:
                control = this.createComboInput(id, value);
            break;
            case this.FormControlTypes.Checkbox:
                control = this.createCheckboxInput(id, value);
            break;
            case this.FormControlTypes.DateTime:
                control = this.createDateTimeInput(id, value);
            break;
            case this.FormControlTypes.Date:
                control = this.createDateInput(id, value);
            break;
            case this.FormControlTypes.Time:
                control = this.createTimeInput(id, value);
            break;
        }
        div.appendChild(labelElement);
        if(control) {
            div.appendChild(control);
        } else {
            div.innerText = " Invalid control type:" + type;
        }
        return {div: div, label: labelElement, control: control};
    },
    createInput(id) {
        let element = document.createElement("input");
        element.id = id;
        return element;
    },
    createTextInput(id, value) {
        let element = this.createInput(id);
        element.type = "text";
        element.value = value;

        element.onchange = function (element, id) {
            if(!element.validity.valid)
                return;
            if(this.debounceer[id])
                clearTimeout(this.debounceer[id]);
            this.debounceer[id] = setTimeout(function(id, element) {
                this.debounceer[id] = undefined;
                if(!element.validity.valid) 
                    return;
                settingChange(id, element.value);
		        this.callBackExtensions(id, element.value, element);
            }.bind(this, id, element), this.debounceerTimeout);
        }.bind(this, element, id);

        return element;
    },
    createNumberInput(id, value) {
        const element = this.createInput(id);
        element.value = parseInt(value);
        element.type = "number";
        element.onchange = function (element, id) {
            if(!element.validity.valid && !element.validity.stepMismatch)
                return;
            if(this.debounceer[id])
                clearTimeout(this.debounceer[id]);
            this.debounceer[id] = setTimeout(function(id, element) {
                this.debounceer[id] = undefined;
                if(!element.validity.valid && !element.validity.stepMismatch)
                    return;
                settingChange(id, parseInt(element.value));
		        this.callBackExtensions(id, element.value, element);
            }.bind(this, id, element), this.debounceerTimeout);
        }.bind(this, element, id);
        return element;
    },
    createDoubleInput(id, value) {
        const element = this.createInput(id);
        element.value = parseFloat(value);
        element.type = "number";
        element.onchange = function (element, id) {
            if(!element.validity.valid && !element.validity.stepMismatch)
                return;
            if(this.debounceer[id])
                clearTimeout(this.debounceer[id]);
            this.debounceer[id] = setTimeout(function(id, element) {
                this.debounceer[id] = undefined;
                if(!element.validity.valid && !element.validity.stepMismatch)
                    return;
                settingChange(id, parseFloat(element.value));
		        this.callBackExtensions(id, element.value, element);
            }.bind(this, id, element), this.debounceerTimeout);
        }.bind(this, element, id);
        return element;
    },
    createTimeInput(id, value) {
        const element = this.createTextInput(id, value);
        element.type = "time";
        return element;
    },
    createDateInput(id, value) {
        const element = this.createTextInput(id, value);
        element.type = "date";
        return element;
    },
    createDateTimeInput(id, value) {
        const element = this.createTextInput(id, value);
        element.type = "datetime-local";
        return element;
    },
    createRadioInput(id, group, checked) {
        let element = this.createInput(id);
        element.type = "radio";
        element.name = group;
        element.checked = checked;
        
        element.onchange = function (element, id) {
            if(!element.validity.valid)
                return;
            if(this.debounceer[id])
                clearTimeout(this.debounceer[id]);
            this.debounceer[id] = setTimeout(function(id, element) {
                this.debounceer[id] = undefined;
                if(!element.validity.valid) 
                    return;
                settingChange(id, element.checked);
		        this.callBackExtensions(id, element.checked, element);
            }.bind(this, id, element), this.debounceerTimeout);
        }.bind(this, element, id);

        return element;
    },
    createCheckboxInput(id, checked) {
        let element = this.createInput(id);
        element.type = "checkbox";
        element.checked = checked;

        element.onchange = function (element, id) {
            if(!element.validity.valid)
                return;
            if(this.debounceer[id])
                clearTimeout(this.debounceer[id]);
            this.debounceer[id] = setTimeout(function(id, element) {
                this.debounceer[id] = undefined;
                if(!element.validity.valid) 
                    return;
                settingChange(id, element.checked);
		        this.callBackExtensions(id, element.checked, element);
            }.bind(this, id, element), this.debounceerTimeout);
        }.bind(this, element, id);

        return element;
    },
    createComboInput(id, values) {
        let element = document.createElement("select");
        element.id = id;
        values.forEach(x => {
            let option = document.createElement("option");
            option.innerText = x["key"];
            option.value = x["value"];
            element.appendChild(option);
        });

        element.onchange = function (element, id) {
            if(this.debounceer[id])
                clearTimeout(debounceer[id]);
            this.debounceer[id] = setTimeout(function(id, element) {
                this.debounceer[id] = undefined;
                if(!element.validity.valid) 
                    return;
                settingChange(id, element.value);
		        this.callBackExtensions(id, element.value, element);
            }.bind(this, id, element), this.debounceerTimeout);
        }.bind(this, element, id);

        return element;
    },
    load(allSettings) {
        if(this.loaded)
            return;
        allSettings["settings"].forEach(x => {
            if(x.internal)
                return;
            this.loadSetting(x);
        });
        this.loaded = true;
    },
    loadSetting(setting) {
        if(this.loaded)
            return;
        let groupElement = document.getElementById(setting.group);
        let appendNewForm = false;
        if(!groupElement) {
            groupElement = this.createForm(setting.group);
            appendNewForm = true;
        }
        switch (setting.profile) {
            case this.SettingProfile.System:
                if(appendNewForm) {
                    systemInfoTab.appendChild(groupElement);
                }
                break;
            default:
                systemError("Invalid settings profile: " + setting.profile);
                return;
        }
        const formControl = this.createFormControl(setting.label, setting.type, setting.key, setting.value);
        groupElement.appendChild(formControl.div);
        this.setSpecialProperties(setting.key, setting.value, formControl);
    },
    setSpecialProperties(key, value, formControl) {
        switch(key) {
            case "playbackRateStep":
                document.getElementById("playbackRateInput").setAttribute("step", value);
                formControl.control.min = 0.0;
                formControl.control.step = 0.01;
                break;
            case "httpChunkSizeMB":
                formControl.control.min = 0.5;
                // formControl.control.step = 1.0;
                break;
            case "scheduleLibraryLoadTime":
                formControl.control.min = "00:00";
                formControl.control.max = "23:59";
                break;
            case "enableMediaManagement":
                mediaManagementEnabled = value;
                break;
                
        }
    },
    callBackExtensions(key, value, formControl) {
        switch(key) {
            case "enableMediaManagement":
                mediaManagementEnabled = value;
	            showChange(showGlobal);
                break;
        }
    }
}