void ftl_get_information(int *free_block_number, int *address_mapping_table)
{
	int i;

	*free_block_number = <your_free_block_number>;
	for (i = 0; i < DATABLKS_PER_DEVICE; ++i)
		address_mapping_table[i] = <your_address_mapping_table[i]>;
}