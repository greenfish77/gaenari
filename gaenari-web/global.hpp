#ifndef HEADER_GLOBAL_H
#define HEADER_GLOBAL_H

path_t path;
option_t option;

// gaenari web console is a microservice.
// only one supul instance is managed and locks are used for thread safety.
// performance requires a farm configuration with multiple microservices.
static supul::supul::supul_t* spl = nullptr;
inline void set_supul(supul::supul::supul_t* s) {spl = s;}
inline supul::supul::supul_t& get_supul(void)   {if (spl) return *spl; ERROR0("internal error");}

#endif // HEADER_GLOBAL_H
