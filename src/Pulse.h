#define SAMPLES 24
#define SLOTS   4



class FilterDC {
    public:
        FilterDC(void)
        {
            average_sample_total = 0;
        }    
        int16_t  filter(int32_t sample)
        {
            average_sample_total += (sample - average_sample_total / SAMPLES);
            return (int16_t)(sample - average_sample_total / SAMPLES);
        }
        int32_t average()
        {
            return average_sample_total/SAMPLES;
        }
        
    private:   
        int32_t average_sample_total;
};


class FilterMA {
    public:
        FilterMA()
        {
            nextslot = 0;
        }
        int16_t filter(int16_t value)
        {
            buffer[nextslot] = value;
            nextslot = (nextslot + 1) % SLOTS;

            int16_t total = 0;
            for (int i = 0; SLOTS>i; ++i)
            {
                total += buffer[i];
            }
            return total / SLOTS;
        }
        
    private:   
        int16_t buffer[SLOTS];
        uint8_t nextslot;
};


class Pulse {
    public:
        Pulse(void);
        int16_t dc_filter(int32_t sample)
        {
            return dc.filter(sample);
        }
        int32_t avgDC()
        {
            return dc.average();
        }
        int16_t ma_filter(int16_t sample)
        {
            return ma.filter(sample);
        }
        
    private:
        FilterDC dc;
        FilterMA ma;
};