#include "OscDevice.h"
#include "ops/strings.h"

#include "Midi.h"
#include "Context.h"

OscDevice::OscDevice(void* _ctx, const std::string& _name, size_t _port) : 
    m_in(nullptr) 
{
    m_type = OSC_DEVICE;
    m_ctx = _ctx;
    m_name = _name;
    m_port = _port;

    
    m_in = new lo::ServerThread(m_port);
    if (m_in->is_valid()) {
        m_in->set_callbacks( [&](){
            std::cout << "Opening OSC server at port " << m_port  << " and filtering address " << m_name << std::endl; 
        }, [](){} );
        m_in->add_method(0, 0, [&](const char *path, lo::Message m) {
            std::string line;
            std::vector<std::string> address = split(std::string(path), '/', false);
            for (size_t i = 0; i < address.size(); i++)
                line +=  ((i != 0) ? "," : "") + address[i];

            unsigned char status = Midi::statusNameToByte(address[ address.size() - 1]); 

            std::string types = m.types();
            lo_arg** argv = m.argv(); 
            lo_message msg = m;

            std::vector<size_t> values;
            for (size_t i = 0; i < types.size(); i++) {
                if ( types[i] == 's')
                    std::cout << "Error: don't know what to do with value: " << std::string( (const char*)argv[i] ) << std::endl;
                else if (types[i] == 'i')
                    values.push_back( (size_t)argv[i]->i );
                else
                    values.push_back( (size_t)argv[i]->f );
            } 

            if (values.size() == 3 ) {
                Context *context = static_cast<Context*>(m_ctx);

                if (context->doKeyExist(m_name, values[0], values[1])) {
                    YAML::Node node = context->getKeyNode(m_name, values[0], values[1]);

                    if (node["status"].IsDefined()) {
                        unsigned char target_status = Midi::statusNameToByte(node["status"].as<std::string>());
                        if (target_status != status)
                            return;
                    }
                    
                    context->configMutex.lock();
                    context->processEvent(node, m_name, status, values[0], values[1], (float)values[2], false);
                    context->configMutex.unlock();
                }

            }
        });
        m_in->start();
    }
    else
        std::cout << "Port " << m_port << " for " << m_name << " is bussy" << std::endl;
}

OscDevice::~OscDevice() {
    close();
}

bool OscDevice::close() {
    if (m_in) {
        m_in->stop();
        delete m_in;
        m_in = nullptr;
    }
    return true;
}