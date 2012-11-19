void solpos_init(struct posdata *pdat, double timezone, int year, int month, int day, int hour, int minute, int second);
void solpos_set_hour(struct posdata *pdat, int hour, int minute, int second);
long solpos_calculate(struct posdata *pdat, double longitude, double latitude);

