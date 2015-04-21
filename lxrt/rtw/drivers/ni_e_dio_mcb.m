
active_8255 = get_param(gcb,'has_8255');
active_daqstc = get_param(gcb,'daqstc_dio');

if (active_daqstc(1:2) == 'on'),
	if (active_8255(1:2) == 'on'),
		set_param(gcb,'MaskEnables',{'on','on','on','on','on','on'});
	else
		set_param(gcb,'MaskEnables',{'on','off','off','on','on','on'});
	end
else
	if (active_8255(1:2) == 'on'),
		set_param(gcb,'MaskEnables',{'on','on','on','on','off','on'});
	else
		set_param(gcb,'MaskEnables',{'on','off','off','on','off','on'});
	end
end
