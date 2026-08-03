file(GLOB_RECURSE scrld ./*.c)
idf_component_register(
  SRCS ${scrld}
  REQUIRES lvgl sd_card_bsp
  INCLUDE_DIRS "custom" "generated" "generated/guider_customer_fonts")
