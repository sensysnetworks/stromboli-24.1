void inp_xxx_init(int port,int nch,char * sName,double p1,
                  double p2, double p3, double p4, double p5);
void inp_xxx_input(int port, double * y, double t);
void inp_xxx_update();
void inp_xxx_end(int port);

void out_xxx_init(int port,int nch,char * sName,double p1,
                  double p2, double p3, double p4, double p5);
void out_xxx_output(int port, double * u,double t);
void out_xxx_end(int port);



