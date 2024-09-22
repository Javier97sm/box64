#if !(defined(GO) && defined(GOM) && defined(GO2) && defined(DATA))
#error Meh....
#endif

GO(jack_free, vFp)
GO(jack_on_shutdown, vFEppp)
GO(jack_port_name, vFv)
GO(jack_connect, vFv)
GO(jack_set_process_callback, vFv)
GO(jack_set_buffer_size_callback, vFv)
GO(jack_port_register, vFv)
GO(jack_midi_event_get, vFv)
GO(jack_port_get_buffer, vFv)
GO(jack_get_ports, vFv)
GO(jack_midi_get_event_count, vFv)
GO(jack_set_port_registration_callback, vFv)
GO(jack_get_sample_rate, vFv
GO(jack_set_sample_rate_callback, vFv)
GO(jack_client_close, vFv)
GO(jack_client_open, vFv)
