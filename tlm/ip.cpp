#ifndef IP_C
#define IP_C
#include "ip.hpp"

SC_HAS_PROCESS(Ip);

Ip::Ip(sc_module_name name) :
    sc_module(name),
    ready(1)
    
{
    SC_THREAD(proc);
    _index.resize(_IndexSize, std::vector<std::vector<num_f>>(_IndexSize, std::vector<num_f>(4, 0.0f)));
    _lookup2.resize(40);
    interconnect_socket.register_b_transport(this, &Ip::b_transport);
    cout << "IP constructed" << endl;
}
    
    
Ip::~Ip()
{
    SC_REPORT_INFO("Ip", "Destroyed");
}


void Ip::b_transport(pl_t& pl, sc_time& offset)
{
    tlm_command cmd = pl.get_command();
    sc_dt::uint64 addr = pl.get_address();
    unsigned char *buf = pl.get_data_ptr();
    unsigned int len = pl.get_data_length();
    pl.set_response_status(TLM_OK_RESPONSE);

    switch (cmd)
    {
        case TLM_WRITE_COMMAND:
            switch(addr)
            {
                case addr_start:
                    start = toInt(buf);
                    proc();
                        cout << "start IP: " << start << endl;
                    break;
                    
                case addr_iradius:
                    iradius = toInt(buf);
                        //cout << "iradius IP: " << iradius << endl;
                    break;	
                case addr_fracr:
                    fracr = toDouble(buf);
                        //cout << "fracr IP: " << fracr << endl;
                    break;
                case addr_fracc:
                    fracc = toDouble(buf);
                        //cout << "fracc IP: " << fracc << endl;
                    break;
                case addr_spacing:
                    inv_spacing = toDouble(buf);
                        //cout << "spacing IP: " << spacing << endl; 
                    break;
                case addr_iy:
                    iy = toInt(buf);
                        //cout << "iy IP: " << iy << endl;
                    break;
                case addr_ix:
                    ix = toInt(buf);
                        //cout << "ix IP: " << ix << endl;
                    break;
                case addr_step:
                    step = toInt(buf);
                        //cout << "step IP: " << step << endl;
                    break;
                case addr_cose:
                    _cose = toDouble(buf);
                        //cout << "_cose IP: " << _cose << endl;
                    break;
                case addr_sine:
                    _sine = toDouble(buf);
                        //cout << "_sine IP: " << _sine << endl;
                    break;
                case addr_scale:
                    scale = toInt(buf);
                        //cout << "scale IP: " << scale << endl;
                    break;
                
                    
                default:
                    pl.set_response_status(TLM_ADDRESS_ERROR_RESPONSE);
                    cout << "Wrong address" << endl;
            }
            break;
                
        case TLM_READ_COMMAND:
            switch(addr)
            {
                case addr_ready:
                    intToUchar(buf, ready);
                    break;
                default:
                    pl.set_response_status(TLM_ADDRESS_ERROR_RESPONSE);
            }
            break;
            
        default:
            pl.set_response_status(TLM_COMMAND_ERROR_RESPONSE);
            cout << "Wrong command" << endl;
    }
    offset += sc_time(10, SC_NS);
}


void Ip::proc() {

    vector<num_f> _lookup2_pom;
    
    for (int n=0; n<40; n++) {
        offset += sc_core::sc_time(DELAY, sc_core::SC_NS);
        _lookup2_pom.push_back(read_rom(addr_rom + n));
    }
    
    for(int i=0; i<40; i++) {
        _lookup2[i] = static_cast<num_f>(_lookup2_pom[i]);
    } 
    
    /*vector<num_f> _lookup2;
    
    for (int n=0; n<40; n++) {
        offset += sc_core::sc_time(DELAY, sc_core::SC_NS);
        _lookup2.push_back(read_rom(addr_rom + n));
    }*/
          
    vector<num_f> pixels1D;
         
    for (int w = 0; w < _width; w++)
    {
        offset += sc_core::sc_time(DELAY, sc_core::SC_NS);
        for (int h = 0; h < _height; h++)
        {
            offset += sc_core::sc_time(DELAY, sc_core::SC_NS);
            pixels1D.push_back( read_mem(addr_Pixels1 + (w * _height + h)));
        }
    }
         
    _Pixels = new num_f*[_width];
    for (int i = 0; i < _width; i++) {
        _Pixels[i] = new num_f[_height];
    }
    
    
    int pixels1D_index2 = 0;
    for (int w = 0; w < _width; w++) {
        for (int h = 0; h < _height; h++) {
            _Pixels[w][h] = static_cast<num_f>(pixels1D[pixels1D_index2++]);
        }
    }  
          
    // Initialize _index array
    for (int i = 0; i < _IndexSize; i++) {
        for (int j = 0; j < _IndexSize; j++) {
            for (int k = 0; k < 4; k++)
                _index[i][j][k] = 0.0;
        }
    }
        
      
    if (start == 1 && ready == 1)
    {
        ready = 0;
        offset += sc_time(DELAY, SC_NS);
    }
    
    else if (start == 0 && ready == 0)
    {
   
        // Examine all points from the gradient image that could lie within the _index square.
        for (int i = 0; i <= 2*iradius; i++) {
            for (int j = 0; j <= 2*iradius; j++) {
    
                /* static int counterfor;
                counterfor++;
                cout << "Uslo u for petlju " << counterfor << " puta" << endl;*/
        
                // Rotate sample offset to make it relative to key orientation.
                // Uses (x,y) coords.  Also, make subpixel correction as later image
                // offset must be an integer.  Divide by spacing to put in _index units.
                rpos = (step * (_cose * (i - iradius) + _sine * (j - iradius)) - fracr) * inv_spacing;

                cpos = (step * (- _sine * (i - iradius) + _cose * (j - iradius)) - fracc) * inv_spacing;

                
                // Compute location of sample in terms of real-valued _index array
                // coordinates.  Subtract 0.5 so that rx of 1.0 means to put full
                // weight on _index[1]
                rx = rpos + 2.0 - 0.5;
                cx = cpos + 2.0 - 0.5;

                // Test whether this sample falls within boundary of _index patch
                if (rx > -1.0 && rx < (double) _IndexSize  &&
                    cx > -1.0 && cx < (double) _IndexSize) {
          
                    num_i r = iy + (i - iradius) * step;
                    num_i c = ix + (j - iradius) * step;
                    num_i ori1, ori2;
                    num_i ri, ci;
                    
                    num_i addSampleStep = int(scale);
                    
                    num_f weight;
                    num_f dxx1, dxx2, dyy1, dyy2;
                    num_f dx, dy;
                    num_f dxx, dyy;
                    
                    num_f rfrac, cfrac;
                    num_f rweight1, rweight2, cweight1, cweight2;
                    
                    
                    if (r >= 1 + addSampleStep && r < _height - 1 - addSampleStep && c >= 1 + addSampleStep && c < _width - 1 - addSampleStep) {
                        weight = _lookup2[num_i(rpos * rpos + cpos * cpos)];
 
                        dxx1 = _Pixels[r + addSampleStep + 1][c + addSampleStep + 1] + _Pixels[r - addSampleStep][c] - _Pixels[r - addSampleStep][c + addSampleStep + 1] - _Pixels[r + addSampleStep + 1][c];
                        dxx2 = _Pixels[r + addSampleStep + 1][c + 1] + _Pixels[r - addSampleStep][c - addSampleStep] - _Pixels[r - addSampleStep][c + 1] - _Pixels[r + addSampleStep + 1][c - addSampleStep];
                        dyy1 = _Pixels[r + 1][c + addSampleStep + 1] + _Pixels[r - addSampleStep][c - addSampleStep] - _Pixels[r - addSampleStep][c + addSampleStep + 1] - _Pixels[r + 1][c - addSampleStep];
                        dyy2 = _Pixels[r + addSampleStep + 1][c + addSampleStep + 1] + _Pixels[r][c - addSampleStep] - _Pixels[r][c + addSampleStep + 1] - _Pixels[r + addSampleStep + 1][c - addSampleStep];

                        dxx = weight * (dxx1 - dxx2);
                        dyy = weight * (dyy1 - dyy2);
                        dx = _cose * dxx + _sine * dyy;
                        dy = _sine * dxx - _cose * dyy;

                        if (dx < 0) ori1 = 0;
                        else ori1 = 1;

                        if (dy < 0) ori2 = 2;
                        else ori2 = 3;

                        if (rx < 0)  ri = 0;
                        else if (rx >= _IndexSize) ri = _IndexSize - 1;
                        else ri = rx;

                        if (cx < 0) ci = 0;
                        else if (cx >= _IndexSize) ci = _IndexSize - 1;
                        else ci = cx;

                        rfrac = rx - ri;
                        cfrac = cx - ci;

                        if (rfrac < 0.0) rfrac = 0.0;
                        else if (rfrac > 1.0) rfrac = 1.0;
                        
                        if (cfrac < 0.0) cfrac = 0.0;
                        else if (cfrac > 1.0) cfrac = 1.0;

                        rweight1 = dx * (1.0 - rfrac);
                        rweight2 = dy * (1.0 - rfrac);
                        cweight1 = rweight1 * (1.0 - cfrac);
                        cweight2 = rweight2 * (1.0 - cfrac);

                        if (ri >= 0 && ri < _IndexSize && ci >= 0 && ci < _IndexSize) {
                            _index[ri][ci][ori1] = cweight1;
                            _index[ri][ci][ori2] = cweight2;
                        }
                    }  
                }
            }
        }

        mem.clear();

        num_f* index1D = new num_f[_IndexSize * _IndexSize * 4];
        
        int index1D_index = 0;
        for (int i = 0; i < _IndexSize; i++) {
            for (int j = 0; j < _IndexSize; j++) {
                for (int k = 0; k < _IndexSize; k++) {
                    index1D[index1D_index++] = static_cast<num_f>(_index[i][j][k]);
                }
            }
        }
                  
        for (long unsigned int i = 0; i < _IndexSize*_IndexSize*4; ++i) 
        {
            mem.push_back(index1D[i]);
        }
                  
        for (int i = 0; i < _IndexSize * _IndexSize * 4; i++)
        {
            pl_t pl;
            offset += sc_core::sc_time(DELAY, sc_core::SC_NS);
            unsigned char* buf;
            buf = (unsigned char*)&mem[i];
            pl.set_address(addr_index1+i);
            pl.set_data_length(1);
            pl.set_data_ptr(buf);
            pl.set_command(tlm::TLM_WRITE_COMMAND);
            pl.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
            mem_socket->b_transport(pl, offset);
         }
                  
         for (int i = 0; i < _width; i++) {
            delete[] _Pixels[i];
         }
         delete[] _Pixels; 
    
         cout << "Entry from IP to memory completed" << endl;
         ready = 1;

    }
    
    wait(1/118.76, SC_NS); // 1/max frekvencija
    
 
}



void Ip::write_mem(sc_uint<64> addr, num_f val) 
{
    pl_t pl;
    unsigned char buf[6];
    doubleToUchar(buf,val);
    pl.set_address(addr);
    pl.set_data_length(1);
    pl.set_data_ptr(buf);
    pl.set_command(tlm::TLM_WRITE_COMMAND);
    pl.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
    mem_socket->b_transport(pl, offset);
}

num_f Ip::read_rom(sc_dt::sc_uint<64> addr)
{
    pl_t pl;
    unsigned char buf[6];
    pl.set_address(addr);
    pl.set_data_length(6);
    pl.set_data_ptr(buf);
    pl.set_command(tlm::TLM_READ_COMMAND);
    pl.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
    rom_socket->b_transport(pl,offset);
    
    num_f mega = toNum_f(buf);
    
    //cout << "buf iz ip-a: " << mega << endl;
    
    return toNum_f(buf);
}

num_f Ip::read_mem(sc_dt::sc_uint<64> addr)
{
    pl_t pl;
    unsigned char buf[6];
    pl.set_address(addr);
    pl.set_data_length(6); 
    pl.set_data_ptr(buf);
    pl.set_command( tlm::TLM_READ_COMMAND );
    pl.set_response_status ( tlm::TLM_INCOMPLETE_RESPONSE );
    mem_socket->b_transport(pl,offset);
    
    num_f mega = toNum_f(buf);
    
    //cout << "buf iz ip-a: " << mega << endl;
    
    return toNum_f(buf);
}

#endif // IP_C
