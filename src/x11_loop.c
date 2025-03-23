#define MOUSE_INNER_BOUNDS 4
#define BILLION 1000000000.0f

void x11_loop(struct x11_context* x11) {
	uint32_t running = 1;
	while(running) 
    {
		x11->input.mouse_delta_x = 0;
		x11->input.mouse_delta_y = 0;
    
    	// TODO - loop - get x11->input, handle other events, call game, render.
		while(XPending(x11->display)) 
    	{
        	struct input_state* input = &x11->input;
			input_reset_buttons(input);        	

    		XEvent e;
    		XNextEvent(x11->display, &e);
    		switch(e.type) 
    		{
        		case Expose:
        		{
            		gl_render(x11, &x11->render_group);
            		break;
        		}
            	case ConfigureNotify:
            	{
                	XWindowAttributes win_attribs;
                	XGetWindowAttributes(x11->display, x11->window, &win_attribs);

                	x11->winw = win_attribs.width;
                	x11->winh = win_attribs.height;
            		glViewport(0, 0, x11->winw, x11->winh);
            	}
            	case MotionNotify:
                {
					if(x11->mouse_just_warped) 
					{
    					x11->mouse_just_warped = 0;
    					break;
					}
                    
					if(!x11->mouse_moved_yet) 
					{
    					x11->mouse_moved_yet = 1;
    					input->mouse_delta_x = 0;
    					input->mouse_delta_y = 0;
        				input->mouse_x = e.xmotion.x;
    					input->mouse_y = e.xmotion.y;
    					break;
					}

					input->mouse_delta_x = e.xmotion.x - input->mouse_x;
					input->mouse_delta_y = e.xmotion.y - input->mouse_y;
					input->mouse_x = e.xmotion.x;
					input->mouse_y = e.xmotion.y;

					if(input->mouse_x < MOUSE_INNER_BOUNDS ||
    					input->mouse_x > x11->winw - MOUSE_INNER_BOUNDS ||
    					input->mouse_y < MOUSE_INNER_BOUNDS ||
    					input->mouse_y > x11->winh - MOUSE_INNER_BOUNDS)
					{
    					x11->mouse_just_warped = 1;
    					input->mouse_x = x11->winw / 2;
    					input->mouse_y = x11->winh / 2;

    					XWarpPointer(
        					x11->display,
        					None,
        					e.xmotion.window,
        					0, 0, 0, 0,
        					x11->winw / 2, x11->winh / 2);
    					XSync(x11->display, 1);
					}

                    break;
                }
        		case KeyPress:
        		{
            		uint32_t keysym = XLookupKeysym(&(e.xkey), 0);
            		switch(keysym)
            		{
                		case XK_Escape:
                		{
                    		running = 0;
        					break;
                		}
                		case XK_w:
                		{
                    		input_button_press(&x11->input.move_forward);
        					break;
                		}
                		case XK_a:
                		{
                    		input_button_press(&x11->input.move_left);
        					break;
                		}
                		case XK_s:
                		{
                    		input_button_press(&x11->input.move_back);
        					break;
                		}
                		case XK_d:
                		{
                    		input_button_press(&x11->input.move_right);
        					break;
                		}
                		default:
                    	{
                        	break;
                        }
            		}
            		break;
        		}
        		case KeyRelease:
            	{
            		uint32_t keysym = XLookupKeysym(&(e.xkey), 0);
            		switch(keysym)
            		{
                		case XK_Escape:
                		{
                    		running = 0;
        					break;
                		}
                		case XK_w:
                		{
                    		input_button_release(&x11->input.move_forward);
        					break;
                		}
                		case XK_a:
                		{
                    		input_button_release(&x11->input.move_left);
        					break;
                		}
                		case XK_s:
                		{
                    		input_button_release(&x11->input.move_back);
        					break;
                		}
                		case XK_d:
                		{
                    		input_button_release(&x11->input.move_right);
        					break;
                		}
                		default:
                    	{
                        	break;
                        }
            		}
            		break;
            	}
        		default:
        		{
            		break;
        		}
    		}
		}

        struct timespec time_cur;
        if(clock_gettime(CLOCK_REALTIME, &time_cur))
        {
    		PANIC();
        }
    	float dt = time_cur.tv_sec - x11->time_prev.tv_sec + 
        	(float)time_cur.tv_nsec / BILLION - (float)x11->time_prev.tv_nsec / BILLION;
        x11->time_prev = time_cur;
    	x11->time_since_start += dt;

		game_update(
    		&x11->render_group, 
    		&x11->input, 
    		dt, 
    		x11->memory_pool, 
    		x11->memory_pool_bytes);

		gl_render(x11, &x11->render_group);

    	// Inside here the game is sent some allocated memory, x11->input state, and
    	// delta time, and asked to return a render list and filled audio buffer.
	}
} 
