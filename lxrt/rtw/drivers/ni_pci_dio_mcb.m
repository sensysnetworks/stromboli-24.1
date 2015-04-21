
active_dio_82C55 = get_param(gcb,'dio_82C55');
active_dio_module = get_param(gcb,'dio_module');

if (active_dio_module(1:2) == 'on'),
	if (active_dio_82C55(1:2) == 'on'),
		set_param(gcb,'MaskEnables',{'on', 'on','on','on','on','on','on'});
	else
		set_param(gcb,'MaskEnables',{'on', 'on','off','off','on','on','on'});
	end
else
	if (active_dio_82C55(1:2) == 'on'),
		set_param(gcb,'MaskEnables',{'on', 'on','on','on','on','off','on'});
	else
		set_param(gcb,'MaskEnables',{'on', 'on','off','off','on','off','on'});
	end
end
