The functions declared in [tfw_p2p_base.hpp](../../../../include/dectnrp/upper/tpoint_firmware/p2p/tfw_p2p_base.hpp), [tfw_p2p_ft.hpp](../../../../include/dectnrp/upper/tpoint_firmware/p2p/tfw_p2p_ft.hpp) and [tfw_p2p_pt.hpp](../../../../include/dectnrp/upper/tpoint_firmware/p2p/tfw_p2p_pt.hpp) are each spread across multiple .cpp files.

1. *_once.cpp:      Function called exactly once, for instance constructors.
2. *_work.cpp:      Contains the tpoint_t work-functions, i.e. work_start_imminent, work_regular, work_pcc etc.
2. *_worksub.cpp:   Functions called from the work-functions. Thus the postfix sub.
